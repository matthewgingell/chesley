////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// search.cpp                                                                 //
//                                                                            //
// Interface to the search engine. Clients are expected to create             //
// and configure search engine objects then call its methods to do            //
// various types of searches.                                                 //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley             //
// the Chess Engine! is free software distributed under the terms             //
// of the GNU Public License.                                                 //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <cassert>
#include <iomanip>
#include <iostream>

#include "chesley.hpp"

using namespace std;

// Constants.
const int Search_Engine :: hist_nbuckets;

// Utility functions.
bool is_mate (Score s) { 
  return abs (s) > MATE_VAL - Search_Engine::MAX_DEPTH; 
}

// Compute the principal variation and return its first move.
Move
Search_Engine :: choose_move (Board &b, int depth)
{
  Move_Vector pv;
  depth = min (depth, MAX_DEPTH);
  new_search (b, depth, pv);
  assert (pv.count > 0);
  return pv[0];
}

/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Search_Engine :: new_search ()                                      //
//                                                                     //
// This is the top level entry point to the tree search. It takes care //
// of initializing the search state and then calls lower level         //
// routines to generate a score and populate the principal variation.  //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

Score
Search_Engine :: new_search
(const Board &b, int depth, Move_Vector &pv)
{  
  // Age the transposition table.
  for (Trans_Table :: iterator i = tt.begin (); i != tt.end(); i++)
    {
      if (i -> second.age >= 5) 
        {
          tt.erase (i);
        }
      else 
        {
          i -> second.age++;
        }
    }

  // Clear history and killer tables. 
  memset (hh_table, 0, sizeof (hh_table));
  memset (killers, 0, sizeof (killers));

  // Clear statistics.
  clear_statistics ();

  // Setup the clock.
  controls.interrupt_search = false;
  if (controls.mode == EXACT || controls.fixed_time >= 0)
    {
      // Set an absolute fixed time limit for this move.
      controls.deadline = mclock () + controls.fixed_time;
    }
  else if (controls.time_remaining >= 0)
    {
      if (controls.moves_remaining > 0)
        {
          // All we do is divide the time available by the moves
          // remaining plus a fudge factor.
          controls.deadline = mclock () +
            (controls.time_remaining / controls.moves_remaining + 10);
        }
      else
        {
          // If time is limited but the move count is not, always
          // assume the game will end in 25 more moves. This should be
          // improved by choosing a limit based on the game phase.
          controls.deadline = mclock () +
            (controls.time_remaining / 25);
        }
    }
  else
    {
      // Some reasonable time control should always be configured.
      assert (0);
    }

  // If one has been configured, set a fixed maximum search depth.
  if (controls.fixed_depth > 0)
    depth = min (depth, controls.fixed_depth);

  // Do the actual tree search.
  return iterative_deepening (b, depth, pv);
}

////////////////////////////////////////////////////////////////////////
//                                                                    //
// Search_Engine :: iterative_deepening                               //
//                                                                    //
// Do a depth first search repeatedly, each time increasing the depth //
// by one. This allows us to return a reasonable move if we are       //
// interrupted.                                                       //
//                                                                    //
////////////////////////////////////////////////////////////////////////

Score
Search_Engine :: iterative_deepening
(const Board &b, int depth, Move_Vector &pv)
{
  Score s = 0;

  // Print header for posting thinking.
  if (post) post_before (b);

  // Search progressively deeper ply until we are interrupted.
  for (int i = 1; i <= depth; i++)
    {
      Move_Vector tmp;

      // Initialize statistics for this iteration.
      calls_to_search = calls_to_qsearch = 0;
      start_time = cpu_time ();

      // Search this position using an aspiration window.
      s = aspiration_search (b, i, tmp, s, 25);

      // Break out of the loop if the search was interrupted.
      if (controls.interrupt_search) break;

      // Otherwise copy the principle variation back to the caller.
      assert (tmp.count > 0);
      pv = tmp;
      if (post) post_each (b, i, pv);

      // Don't bother searching any further if we've found a
      // checkmate.
      if (is_mate (s)) break;
    }

  // Write out statistics about this search.
  if (post) post_after ();

  // Check that we got at least one move.
  assert (pv.count > 0);

  return pv[0].score;
}

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Search_Engine :: aspiration_search ()                                   //
//                                                                         //
// Try a search with a window around 'best_guess' and only do a full width //
// search if that fails.                                                   //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

Score
Search_Engine :: aspiration_search
(const Board &b, int depth, Move_Vector &pv, Score best_guess, Score hw)
{
  Score s;

#ifdef ENABLE_ASPIRATION_WINDOW
  const Score lower = best_guess - hw;
  const Score upper = best_guess + hw;

  s = search_with_memory (b, depth, 0, pv, lower, upper);

  // Search from scratch if we failed.
  if (s <= lower || s >= upper)
    {
      pv.clear ();
      s = search_with_memory (b, depth, 0, pv);
    }
#else
    {
      s = search_with_memory (b, depth, 0, pv);
    }
#endif  /* ENABLE_ASPIRATION_WINDOW */

  return s;
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Search_Engine :: search_with_memory ()                               //
//                                                                      //
// This routine wraps search and memoizes look ups in the               //
// transposition table.                                                 //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

Score
Search_Engine :: search_with_memory
(const Board &b,
 int depth, int ply,
 Move_Vector &pv,
 Score alpha, Score beta,
 bool do_null_move)
{
  assert (pv.count == 0);

  Move m;
  Score original_alpha = alpha;
  Score original_beta = beta;

  // Try to find this node in the transposition table.
  if (ply > 0 && tt_try (b, depth, m, alpha, beta))
    {
      pv.push (m);
      return m.score;
    }

  // After narrowing alpha and beta from the transposition table, we
  // may sometimes end up with null bounds. In that case return
  // immediately.
  if (alpha >= beta) return alpha;

  // Push this position on the repetition stack and recurse.
  rt_push (b);
  Score s = search (b, depth, ply, pv, alpha, beta, do_null_move);

  // We must always return at least one move at ply 0. Because the
  // search is unstable our top level search may cut off because the
  // bounds stored in the transposition do not yeild a PV node. In
  // this case, we search again with the original window.
  if (ply == 0 && pv.count == 0)
    {
      s = search
        (b, depth, ply, pv, original_alpha, original_beta, do_null_move);
    }

  rt_pop (b);

  // Update the transposition table if this search returned a PV.
  if (pv.count > 0 && !controls.interrupt_search)
    tt_update (b, depth, pv[0], alpha, beta);

  return s;
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Search_Engine :: search ()                                           //
//                                                                      //
// This is the negamax implementation at the core of search hierarchy.  //
///                                                                     //
//////////////////////////////////////////////////////////////////////////

Score
Search_Engine :: search
(const Board &b,
 int depth, int ply,
 Move_Vector &pv,
 Score alpha, Score beta,
 bool do_null_move)
{
  assert (pv.count == 0);
  assert (alpha < beta);

  int legal_move_count = 0;
  bool in_check = b.in_check ((b.to_move ()));

  // Check 50 move and triple repetition rules.
  if (b.half_move_clock == 50 || is_rep (b))
    return 0;

  // Abort if we have been interrupted.
  if (controls.interrupt_search)
    return 12345;

  // Update statistics.
  calls_to_search++;

  // Return the result of a quiescence search at depth 0.
  if (depth <= 0)
    {
      // Check whether this position is checkmate.
      if (in_check && b.child_count () == 0)
        {
          return -(MATE_VAL - ply);
        }
      else
        {
#ifdef ENABLE_QSEARCH
          alpha = qsearch (b, -1, ply, alpha, beta);
#else
          alpha = Eval (b).score (alpha, beta);
#endif /* ENABLE_QSEARCH */
        }
    }

  // Otherwise recurse over the children of this node.
  else {

#ifdef ENABLE_NULL_MOVE
    //////////////////////////
    // Null move heuristic. //
    //////////////////////////

    const int R = (depth > 6) ? 3 : 2;

    // Since we don't have Zugzwang detection we just disable null
    // move if there are fewer than 10 pieces on the board.
    if (ply > 1 && do_null_move && pop_count (b.occupied) >= 15 && !in_check)
      {
        Move_Vector dummy;
        Board c = b;
        c.set_color (invert (c.to_move ()));
        int val = -search_with_memory
          (c, depth - R - 1, ply + 1, dummy, -beta, -beta + 1, false);
        if (val >= beta) return val;
        c.set_color (invert (b.to_move ()));
      }
#endif // ENABLE_NULL_MOVE

    // Generate moves.
    Move_Vector moves (b);
    order_moves (b, depth, moves);

    ////////////////////////////
    // Minimax over children. //
    ////////////////////////////

    int mi = 0;
    bool have_pv_move = false;
    for (mi = 0; mi < moves.count; mi++)
      {
        int cs;
        Board c = b;
        Move_Vector cpv;

        // Skip this move if it's illegal.
        if (!c.apply (moves[mi])) continue;

        legal_move_count++;

        ////////////////////////
        // Search extensions. //
        ////////////////////////

        int ext = 0;
#ifdef ENABLE_EXTENSIONS

        // Check extensions.
        if (in_check) ext += 1;

        // Pawn to seventh rank extensions.
        int rank = Board :: idx_to_rank (moves[mi].to);
        if ((rank == 1 || rank == 6) &&
            moves[mi].get_kind (b) == PAWN)
          ext+= 1;

#endif // ENABLE_EXTENSIONS

#ifdef ENABLE_LMR
        ///////////////////////////
        // Late move reductions. //
        ///////////////////////////

        const int Full_Depth_Count = 4;
        const int Reduction_Limit = 3;
        if (mi >= Full_Depth_Count && 
            depth >= Reduction_Limit &&
            ext == 0 && 
            moves[mi].capture (b) == NULL_KIND)
          {
            int ds;
            Move_Vector dummy;
            ds = -search_with_memory
              (c, depth - 2, ply + 1, dummy, -(alpha + 1), -alpha, true);

            // If we fail low, we are done with this move.
            if (ds <= alpha) continue;
          }
#endif // ENABLE_LMR

#if ENABLE_PVS
        /////////////////////////////////
        // Principle variation search. //
        /////////////////////////////////
        if (have_pv_move)
          {
            cs = -search_with_memory
              (c, depth - 1 + ext, ply + 1, cpv, -(alpha + 1), -alpha, true);
            if (cs > alpha && cs < beta)
              {
                cpv.clear ();
                cs = -search_with_memory
                  (c, depth - 1 + ext, ply + 1, cpv, -beta, -alpha, true);
              }
          }
        else
#endif // ENABLE_PVS

       // Search this child recursively.
          {
            cs = -search_with_memory
              (c, depth - 1 + ext, ply + 1, cpv, -beta, -alpha, true);
          }

        // Test whether this move is better than the moves we have
        // previously analyzed.
        if (cs > alpha)
          {
            alpha = cs;
            moves[mi].score = alpha;
            pv = Move_Vector (moves[mi], cpv);
            if (alpha < beta) 
              have_pv_move = true;
            else
              break;
          }
      }

    // Collect statistics.
    hist_pv[min (mi, hist_nbuckets)]++;

    // If we couldn't find a move that applied, then the game is over.
    if (legal_move_count == 0)
      {
        if (in_check)
          {
            alpha = -(MATE_VAL - ply);
          }
        else
          {
            alpha = 0;
          }
      }
    else
      {
        // If we found a move, this is either a PV or a fail high move.
        if (pv.count > 0)
          {
            hh_table[b.to_move ()][depth][pv[0].from][pv[0].to] += 1;            
            if (alpha > beta && pv[0] != killers[depth][0])
              {
                killers[depth][1] = killers[depth][0];
                killers[depth][0] = pv[0];
              }
          }
      }
  }

  return alpha;
}

// Attempt to order moves to improve our odds of getting earlier
// cutoffs.
void
Search_Engine :: order_moves
(const Board &b, int depth, Move_Vector &moves) {
  TT_Entry e;
  bool have_entry = tt_fetch (b.hash, e);
  Move best_guess = Move (0);

  // If we have an entry in the transposition table, use that as our
  // best guess.
  if (have_entry)
    {
      best_guess = e.move;
    }

  for (int i = 0; i < moves.count; i++)
    {
      assert (moves[i].score == 0);

      // Sort our best guess first.
      if (moves[i] == best_guess)
        {
          moves[i].score = +INF;
          continue;
        }

      // Followed by promotions to queen.
      if (moves[i].promote == QUEEN)
        moves[i].score += 1000;

      // Followed by captures.
      if (moves[i].capture(b) != NULL_KIND)
        moves[i].score += 100 * eval_piece (moves[i].capture (b));
      
      // Followed by PV and fail high nodes.
#if 0
      if (moves[i] == killers[depth][0]) moves[i].score += 1000;
      if (moves[i] == killers[depth][1]) moves[i].score += 1000;
#endif

      uint64 hh_val = hh_table[b.to_move ()][depth][moves[i].from][moves[i].to];
      moves[i].score += hh_val;
    }

  insertion_sort <Move_Vector, Move, less_than> (moves);
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Search_Engine :: see ()                                          //
//                                                                  //
// Static exchange evaluation. This routine plays out a series of   //
// captures in least-valuable attacker order, stopping when all     //
// captures are resolved or a capture is disadvantageous for the    //
// moving side. The goal here is to generate a good estimate of the //
// value of a capture in linear time.                               //
//                                                                  //
//////////////////////////////////////////////////////////////////////

Score
Search_Engine :: see (const Board &b, const Move &m) {
  Score s = eval_piece (m.capture (b));

  // Construct child position.
  Board c = b;
  c.clear_piece (m.from);
  c.clear_piece (m.to);
  c.set_piece (m.get_kind (b), b.to_move (), m.to);
  c.set_color (invert (b.to_move ()));

  Move lvc = c.least_valuable_attacker (m.to);
  if (!lvc.is_null () && b.get_kind (m.from) != KING)
    {
      assert (lvc.to == m.to);
      s += -max (see (c, lvc), 0);
    }
  return s;
}

// Quiescence search.
Score
Search_Engine :: qsearch
(const Board &b, int depth, int ply,
 Score alpha, Score beta)
{
  Move_Vector dummy;

  // Update statistics.
  calls_to_qsearch++;

  // Do static evaluation at this node.
  alpha = max (alpha, Eval (b).score (alpha, beta));

  // Recurse and minimax over children.
  if (alpha < beta)
    {
      Board c;
      Move_Vector moves;

      /////////////////////////////////
      // Generate and sort captures. //
      /////////////////////////////////

      b.gen_captures (moves);
      for (int i = 0; i < moves.count; i++)
        {
#ifdef ENABLE_SEE
          moves[i].score = see (b, moves[i]);
#else
          moves[i].score = eval_piece (moves[i].capture (b));
#endif
        }
      insertion_sort <Move_Vector, Move, less_than> (moves);

      // Minimax over captures.
      for (int i = 0; i < moves.count; i++)
        {
          c = b;
          if (c.apply (moves[i]))
            {
              alpha = max
                (alpha, -qsearch (c, depth - 1, ply + 1, -beta, -alpha));
              if (alpha >= beta)
                break;
            }
        }
    }

  return alpha;
}

////////////////////////////////////////////////////////////////////////
//                                                                    //
// Transposition tables.                                              //
//                                                                    //
// Operations on the transposition table.                             //
//                                                                    //
////////////////////////////////////////////////////////////////////////

// Try to get a move or tighten the window from the transposition
// table, returning true if we found a move we can return at this
// position.
inline bool
Search_Engine :: tt_try
(const Board &b, int32 depth, Move &m, int32 &alpha, int32 &beta)
{
#if ENABLE_TRANS_TABLE
  Trans_Table :: iterator i = tt.find (b.hash);

  if (i == tt.end ())
    {
      tt_misses++;
      return false;
    }
  else
    {
      tt_hits++;
      i -> second.age = 0;
      if (i -> second.depth >= depth &&
          b.half_move_clock < 45  && 
          rep_count (b) == 0)
        {
          TT_Entry entry = i -> second;

          if (entry.type == LOWERBOUND)
            alpha = max (entry.move.score, alpha);

          else if (entry.type == UPPERBOUND)
            beta = min (entry.move.score, beta);

          else if (entry.type == EXACT_VALUE)
            {
              m = entry.move;
              return true;
            }
        }
    }
#endif

  return false;
}

// Fetch an entry from the transposition table. Returns false if no
// entry is found.
inline bool
Search_Engine :: tt_fetch (uint64 hash, TT_Entry &out) {
#if ENABLE_TRANS_TABLE
  Trans_Table :: iterator i = tt.find (hash);
  if (i != tt.end ())
     {
       i -> second.age = 0;
       out = i -> second;
       return true;
     }
  else
#endif // ENABLE_TRANS_TABLE
    {
      return false;
    }

}

// Update the transposition table with the results of a call to
// search.
inline void
Search_Engine :: tt_update
(const Board &b, int32 depth, const Move &m, int32 alpha, int32 beta)
{
#if ENABLE_TRANS_TABLE
  Trans_Table :: iterator i = tt.find (b.hash);
  bool entry_is_new = (i == tt.end ());

  // Insert an element if it's new.
  if (entry_is_new)
    {
      Trans_Table :: value_type val (b.hash, TT_Entry ());
      i = tt.insert (val).first;
    }

  // Determine the kind of value we are recording.
  Node_Type type;
  if (m.score >= beta) 
    {
      type = LOWERBOUND;
    }
  else if (m.score <= alpha)
    {
      type = UPPERBOUND;
    }
  else
    {
      type = EXACT_VALUE;
    }

  // Use an 'always replace' scheme.
  i -> second.move = m;
  i -> second.depth = depth;
  i -> second.age = 0;
  i -> second.type = type;

#endif // ENABLE_TRANS_TABLE
}

////////////////////////////////////////////////////////////////////////
// Repetition tables.                                                 //
//                                                                    //
// Operations on repetition tables. These are used to enforce the     //
// triple repetition rule.                                            //
////////////////////////////////////////////////////////////////////////

// Add an entry to the repetition table.
void
Search_Engine :: rt_push (const Board &b) {
  Rep_Table :: iterator i = rt.find (b.hash);

  assert (rt.size () < 1000);

  if (i == rt.end ())
    {
      rt[b.hash] = 1;
    }
  else
    {
      i -> second += 1;
    }
}

// Remove an entry to the repetition table.
void
Search_Engine :: rt_pop (const Board &b) {
  Rep_Table :: iterator i = rt.find (b.hash);
  assert (i != rt.end ());
  i -> second -= 1;
  if (i -> second == 0) rt.erase (b.hash);
}

// Fetch the repetition count for a position.
int
Search_Engine :: rep_count (const Board &b) {
  Rep_Table :: iterator i = rt.find (b.hash);
  if (i == rt.end ())
    {
      return 0;
    }
  else
    {
      return i -> second;
    }
}

// Test whether this board is a repetition.
bool
Search_Engine :: is_rep (const Board &b) {
  return rep_count (b) > 1;
}

// Test whether this board is a third repetition.
bool
Search_Engine :: is_triple_rep (const Board &b) {
  Rep_Table :: iterator i = rt.find (b.hash);

  if (i == rt.end ())
    {
      return false;
    }
  else
    {
      int count = i -> second;
      assert (count >= 0 && count < 4);
      return (count == 3);
    }
}

////////////////////////////////////////////////
// Time control.                              //
//                                            //
// Routines for managing game clock resource. //
////////////////////////////////////////////////

// Method polled periodically by the owner of the Search_Engine
// object.
void
Search_Engine :: poll (uint64 clock) {
  if (clock >= controls.deadline)
    {
      controls.interrupt_search = true;
    }
}


// Set fixed time per move in milliseconds.
void
Search_Engine :: set_fixed_depth (int depth) {
  controls.fixed_depth = depth;
  return;
}

// Set fixed depth per move.
void
Search_Engine :: set_fixed_time (int time) {
  controls.mode = EXACT;
  controls.fixed_time = time;
  controls.moves_ptc = -1;
  controls.time_ptc = -1;
  controls.increment = -1;
  controls.time_remaining = -1;
  controls.moves_remaining = -1;
  return;
}

// Set up time controls corresponding to and xboard "level" command.
void
Search_Engine :: set_level (int mptc, int tptc, int inc) {
  controls.mode = CONVENTIONAL;
  controls.moves_ptc = mptc;
  controls.time_ptc = tptc;
  controls.fixed_time = -1;
  controls.increment = inc;
  if (inc > 0) controls.mode = ICS;
  if (tptc > 0) controls.time_remaining = tptc;
  if (mptc > 0) controls.moves_remaining = mptc;
}

// Set the time remaining before the game clock expires.
void
Search_Engine :: set_time_remaining (int msecs) {
  controls.time_remaining = msecs;
}

////////////////////////////////////////////////////////////////////////
// Thinking output.                                                   //
//                                                                    //
// Output the principal variation for each individual search depth    //
// and write out some performance statistics.                         //
////////////////////////////////////////////////////////////////////////

// Write thinking output before iterative deeping starts.
void
Search_Engine :: post_before (const Board &b) {
  cout << "Move " << b.full_move_clock << ":" << b.half_move_clock << endl
       << "Ply    Eval    Time    Nodes   Principal Variation"
       << endl;
}

// Write thinking for each depth during iterative deeping.
void
Search_Engine :: post_each (const Board &b, int depth, const Move_Vector &pv) {
  double elapsed = ((double) cpu_time () - (double) start_time) / 1000;
  Board c = b;

  // Xboard format is: ply, score, time, nodes, pv.

  cout << setiosflags (ios :: right);

  // Ply.
  cout << setw (3) << depth;

  // Scorce
  Score score = pv[0].score;
  if (is_mate (score))
    {
      cout << setw (2) << (score > 0 ? "+" : "-");
      cout << setw (4) << "Mate";
      cout << setw (2) << MATE_VAL - abs(score);
    }
  else
    {
      cout << setw (8) << pv[0].score;
    }

  // Time elapsed.
  cout << setw (8) << setiosflags (ios :: fixed) << setprecision (2);
  cout << elapsed;

  // Node count.
  cout << setw (9);
  cout << calls_to_search + calls_to_qsearch;
  cout << "   ";
  
  // Principle variation.
  for (int i = 0; i < pv.count; i++)
    {
      cout << c.to_san (pv[i]) << " ";
      c.apply (pv[i]);
    }

  cout << endl;
}

// Write thinking output after iterative deeping ends.
void
Search_Engine :: post_after () {
  // Display statistics on the quality of our move ordering.
  cout << endl;
  double sum = 0;
  for (int i = 0; i < hist_nbuckets; i++)
    sum += hist_pv[i];
  cout << "pv hist: ";
  for (int i = 0; i < hist_nbuckets; i++)
    cout << (hist_pv[i] / sum) * 100 << "% ";
  cout << endl;

  // Display statistics on transposition table hit rate.
  double hit_rate = (double) tt_hits / (tt_hits + tt_misses);
  cout << "tt hit rate: " << hit_rate * 100 << "%" << endl;

  // Display transposition table size.
  cout << "tt entries: " << tt.size () << endl;
}
