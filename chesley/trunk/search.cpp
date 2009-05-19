////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// search.cpp                                                                 //
//                                                                            //
// Interface to the search engine. Clients are expected to create and         //
// configure search engine objects then call its methods to do various        //
// types of searches.                                                         //
//                                                                            //
// Matthew Gingell                                                            //
// gingell@adacore.com                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <cassert>
#include <iomanip>
#include <iostream>

#include "chesley.hpp"

using namespace std;

// Compute the principal variation and return its first move.
Move
Search_Engine :: choose_move (Board &b, int depth) 
{
  Move_Vector pv;
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
  // Age the history table.
  for (uint32 i = 0; i < sizeof (hh_table) / sizeof (uint64); i++)
    ((uint64 *) hh_table)[i] /= 2;
  
  // Age the transposition table.
  cerr << "ageing..." << endl;
  Trans_Table :: iterator i;
  for (i = tt.begin (); i != tt.end(); i++)
    {
      if (i -> second.age >= 5)
	tt.erase (i);
      else 
	i -> second.age++;
    }
  cerr << "done..." << endl;
  
  // Clear statistics.
  calls_to_search = 0;
  calls_to_qsearch = 0;
  memset (hist_pv, 0, sizeof (hist_pv));
  tt_hits = 0;
  tt_misses = 0;

  return iterative_deepening (b, depth, pv);
}

////////////////////////////////////////////////////////////////////////
//                                                                    //
// Search_Engine::iterative_deepening                                 //
//                                                                    //
// Do a depth first search repeatedly, each time increasing the depth //
// by one. This allows us to return a reasonable move if we are       //
// interrupted.                                                       //
//                                                                    //
////////////////////////////////////////////////////////////////////////

Score 
Search_Engine::iterative_deepening 
(const Board &b, int depth, Move_Vector &pv) 
{
  Score s = 0;
  Score scores[100];

  // Print header for posting thinking.
  if (post) post_before (b);

  // Search progressively deeper ply until we are interrupted.
  for (int i = 1; i <= depth; i++) 
    {
      Move_Vector tmp;

      // Initialize statistics for this iteration.
      calls_to_search = calls_to_qsearch = 0;
      start_time = cpu_time ();

      // Search this position using a dynamically sized aspiration window.
      int delta = 
	(i > 2) ? (abs(scores[i - 1] - scores[i - 2])) + 10 : INF;
      scores[i] = s = aspiration_search (b, i, tmp, s, delta);

      // Break out of the loop if the search was interrupted.
      if (interrupt_search) 
        break;

      // Otherwise, if the search completed and we got back a
      // principle variation, copy it back to the caller.
      if (tmp.count > 0)
        {
          pv = tmp;
          if (post) post_each (b, i, pv);
        }
    }

  // Write out statistics about this search.
  if (post) post_after ();

  // If we got back no principal variation, return a depth 1 search.
  if (pv.count == 0) {
    interrupt_search = false;
    search_with_memory  (b, 1, 0, pv, -INF, INF);    
    interrupt_search = true;
  }

  // Check that we got at least one move.
  assert (pv.count > 0);
  
  return pv[0].score;
}

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Search_Engine::aspiration_search ()                                     //
//                                                                         //
// Try a search with a window around 'best_guess' and only do a full width //
// search if that fails.                                                   //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

Score 
Search_Engine::aspiration_search 
(const Board &b, int depth, Move_Vector &pv, Score best_guess, Score hw) 
{
  Score s;

#ifdef ENABLE_ASPIRATION_WINDOW
  const Score lower = best_guess - hw;
  const Score upper = best_guess + hw;

  s = search_with_memory  (b, depth, 0, pv, lower, upper);

  // Re-search if we failed.
  if (s <= lower || s >= upper)
    {
      pv.clear ();
      s = search_with_memory (b, depth, 0, pv);
    }
#else
  s = search_with_memory (b, depth, 0, pv);
#endif /* ENABLE_ASPIRATION_WINDOW */

  return s;
}

/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Search_Engine :: search_with_memory ()                              //
//                                                                     //
// This routine wraps search and memoizes lookups in the transposition //
// table.                                                              //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

Score
Search_Engine :: search_with_memory 
(const Board &b, 
 int depth, int ply, 
 Move_Vector &pv, 
 Score alpha, Score beta,
 bool do_null_move)
{
  assert (pv.count == 0);

  // Try to find this node in the transposition table.
  Move m;  
  if (tt_try (b, depth, m, alpha, beta))
    {
      pv.push (m);
      return m.score;
    }
  
  // Push this position on the repetition stack and recurse.
  rt_push (b);
  Score s = search (b, depth, ply, pv, alpha, beta, do_null_move);
  rt_pop (b);
  
  // Update the transposition table if this search returned a PV.
  if (pv.count > 0)
    tt_update (b, depth, pv[0], alpha, beta);
  
  return s;  
}

Score
Search_Engine :: search 
(const Board &b, 
 int depth, int ply, 
 Move_Vector &pv, 
 Score alpha, Score beta,
 bool do_null_move)
{
  bool legal_move_count = 0;
  bool in_check = b.in_check ((b.to_move ()));

  assert (pv.count == 0);

  ////////////////////////////////////////////////
  // Check 50 move and triple repitition rules. //
  ////////////////////////////////////////////////

  if (b.half_move_clock == 50 || is_rep (b))
    return 0;

  ////////////////////////////////////////
  // Abort if we have been interrupted. //
  ////////////////////////////////////////
  
  if (interrupt_search)
    return 12345;

  ////////////////////////
  // Update statistics. //
  ////////////////////////

  calls_to_search++;

  //////////////////////////////////////////////////////////
  // Return the result of a quiescence search at depth 0. //
  //////////////////////////////////////////////////////////
  
  if (depth <= 0) 
    {
#ifdef ENABLE_QSEARCH
      alpha = qsearch (b, -1, ply, alpha, beta);
#else
      alpha = Eval (b).score ()
#endif /* ENABLE_QSEARCH */
    }

  ///////////////////////////////////////////////////////
  // Otherwise recurse over the children of this node. //
  ///////////////////////////////////////////////////////
  
  else {

#ifdef ENABLE_NULL_MOVE
    //////////////////////////
    // Null move heuristic. //
    //////////////////////////

    const int R = 2;

    // Since we don't have Zugzwang detection we just disable null
    // move if there are fewer than 15 pieces on the board.
    if (ply > 1 && 
	do_null_move && 
	pop_count (b.occupied) >= 15 && 
	!in_check)
      {
        Board c = b;
        Move_Vector dummy;
        c.set_color (invert (b.to_move ()));
        int val = -search_with_memory 
          (c, depth - R - 1, ply + 1, dummy, -beta, -beta + 1, false);
        if (val >= beta)
	  return val;
      }
#endif /* ENABLE_NULL_MOVE */
    
    // Generate moves.
    Move_Vector moves (b);

#ifdef ENABLE_ORDER_MOVES
    order_moves (b, depth, moves, alpha, beta);
#endif

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
        if (c.apply (moves[mi]))
          {
            legal_move_count++;

	    ////////////////////////
            // Search extensions. //
            ////////////////////////
	    
	    int ext = 0;
	    if (in_check) 
	      ext += 1;
	    
	    if (moves[mi].get_kind (b) == PAWN && 
		(moves[mi].to == 1 || moves[mi].to == 6)) 
	      ext += 1;

	    if (moves[mi].promote == QUEEN) 
	      ext += 1;

#ifdef ENABLE_LMR
	    ///////////////////////////
            // Late move reductions. //
            ///////////////////////////

	    const int Full_Depth_Count = 4;
	    const int Reduction_Limit = 3;
	    if (mi >= Full_Depth_Count && 
		depth >= Reduction_Limit && 
		ext == 0 && 
		have_pv_move)
	      {
		int ds;
		Move_Vector dummy;
		ds = -search_with_memory 
		  (c, depth - 2, ply + 1, dummy, -(alpha + 1), -alpha, true);
		
		// If we fail low, we are done with this node.
		if (ds <= alpha)
		  continue;
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
	      {
		cs = -search_with_memory 
		  (c, depth - 1 + ext, ply + 1, cpv, -beta, -alpha, true);
	      }
            
            // Test whether this move is better than the moves we have
            // previously analyzed.
            if (cs > alpha)
              {
                if (cs < beta) have_pv_move = true;
                alpha = cs;
                moves[mi].score = alpha;
                pv = Move_Vector (moves[mi], cpv);
              }   

#ifdef ENABLE_ALPHA_BETA
	    /////////////////////////
            // Test for fail high. //
            /////////////////////////
	    
            if (cs >= beta)
              {
                break;
              }
#endif // ENABLE_ALPHA_BETA
          }
      }

    // Collect statistics.
    hist_pv[min (mi, hist_nbuckets)]++;

    // If we couldn't find a move that applied, then the game is over.
    if (legal_move_count == 0)
      {
        if (in_check)
          {
            ///////////////////////////////////////////////////////////
            // We need to provide a bonus to shallower wins over     //
            // deeper ones, since if we're ambivalent between near   //
            // and far wins at every ply the search will happily     //
            // prove it can mate in N forever and never actually get //
            // there.                                                //
            ///////////////////////////////////////////////////////////

            alpha = -(MATE_VAL - ply);
          }
        else
          {
            alpha = 0;
          }
      }
    else
      {
        ////////////////////////////////////////////////
        // Update the history table with this result. //
        ////////////////////////////////////////////////
        if (pv.count > 0)
          hh_table[b.to_move ()][depth][pv[0].from][pv[0].to] += 1;
      }
  }

  return alpha;
}

// Attempt to order moves to improve our odds of getting earlier
// cutoffs.
void 
Search_Engine::order_moves 
(const Board &b, int depth, Move_Vector &moves, int alpha, int beta) {
  TT_Entry e;
  bool have_entry = tt_fetch (b.hash, e);
  Move best_guess;

  // If we have an entry in the transposition table, use that as our
  // best guess.
  if (have_entry)
    {
      best_guess = e.move;
    }

  // Otherwise find a best guess using a reduced depth search.
  else
    {
      Move_Vector pv;
      search_with_memory (b, depth - 3, 0, pv, alpha, beta);
      if (pv.count > 0) best_guess = pv [0];
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
      moves[i].score += 100 * eval_piece (moves[i].capture (b));
      
      // And finally, recent PV and fail-high moves.
      moves[i].score += 
        hh_table[b.to_move ()][depth][moves[i].from][moves[i].to];    
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
Search_Engine::see (const Board &b, const Move &m) {
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
Search_Engine::qsearch 
(const Board &b, int depth, int ply, 
 Score alpha, Score beta) 
{
  Move_Vector dummy;

  ////////////////////////
  // Update statistics. //
  ////////////////////////

  calls_to_qsearch++;

  ////////////////////////////////////////
  // Do static evaluation at this node. //
  ////////////////////////////////////////

  alpha = max (alpha, Eval (b).score ());

  ////////////////////////////////////////
  // Recurse and minimax over children. // 
  ////////////////////////////////////////

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
      
      ////////////////////////////
      // Minimax over captures. //
      ////////////////////////////

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
Search_Engine::tt_try 
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
      if (i -> second.depth == depth && 
          b.half_move_clock < 45 && rep_count (b) == 0)
        {
          TT_Entry entry = i -> second;

          if (entry.type == TT_Entry::LOWERBOUND)
            alpha = max (entry.move.score, alpha);

          else if (entry.type == TT_Entry::UPPERBOUND)
            beta = min (entry.move.score, beta);
          
          else if (entry.type == TT_Entry::EXACT_VALUE)
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
Search_Engine::tt_fetch (uint64 hash, TT_Entry &out) {
#if ENABLE_TRANS_TABLE
  Trans_Table::iterator i = tt.find (hash);
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
Search_Engine::tt_update
(const Board &b, int32 depth, const Move &m, int32 alpha, int32 beta) 
{
#if ENABLE_TRANS_TABLE
  Trans_Table :: iterator i = tt.find (b.hash);
  bool entry_is_new = (i == tt.end ());

  // Insert an element if it's new.
  if (entry_is_new)
    {
      Trans_Table::value_type val (b.hash, TT_Entry ());
      i = tt.insert (val).first;
    }
  
  // Populate it if it's new or deeper than the existing entry.
  //  if (entry_is_new || depth >= i -> second.depth) */

  // Always replace
  {
    i -> second.move = m;
    i -> second.depth = depth;
    i -> second.age = 0;
    if (m.score >= beta) 
      i -> second.type = TT_Entry :: LOWERBOUND;
    else if (m.score <= alpha)
      i -> second.type = TT_Entry :: UPPERBOUND;
    else
      i -> second.type = TT_Entry :: EXACT_VALUE;
  }

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
Search_Engine::rt_push (const Board &b) {
  Rep_Table::iterator i = rt.find (b.hash);

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
Search_Engine::rt_pop (const Board &b) {
  Rep_Table::iterator i = rt.find (b.hash);
  assert (i != rt.end ());
  i -> second -= 1;
  if (i -> second == 0) rt.erase (b.hash);
}

// Fetch the repetition count for a position. 
int
Search_Engine::rep_count (const Board &b) {
  Rep_Table::iterator i = rt.find (b.hash);
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
Search_Engine::is_rep (const Board &b) {
  return rep_count (b) > 1;
}

// Test whether this board is a third repetition.
bool
Search_Engine::is_triple_rep (const Board &b) {
  Rep_Table::iterator i = rt.find (b.hash);

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

////////////////////////////////////////////////////////////////////////
// Thinking output.                                                   //
//                                                                    //
// Output the principal variation for each individual search depth    // 
// and write out some performance statistics.                         //
////////////////////////////////////////////////////////////////////////

// Write thinking output before iterative deeping starts.
void 
Search_Engine::post_before (const Board &b) {
  cerr << "Move " << b.full_move_clock << ":" << b.half_move_clock << endl
       << "Ply     Nodes    Qnodes    Time    Eval   Principal Variation" 
       << endl;
}

// Write thinking for each depth during iterative deeping.
void 
Search_Engine::post_each (const Board &b, int depth, const Move_Vector &pv) {
  double elapsed = ((double) cpu_time () - (double) start_time) / 1000;
  Board c = b;

  // Write out ply, node count, qnode count, time, score, and pv;
  cerr << setw (3)  << depth;
  cerr << setw (10) << calls_to_search;
  cerr << setw (10) << calls_to_qsearch;
  cerr << setw (8)  << setiosflags (ios::fixed) << setprecision (2) << elapsed;
  cerr << setw (8)  << pv[0].score  << "   ";
  
  for (int i = 0; i < pv.count; i++)
    {
      cerr << c.to_san (pv[i]) << " ";
      c.apply (pv[i]);
    }
  cerr << endl;
}

// Write thinking output after iterative deeping ends.
void 
Search_Engine::post_after () {
  // Display statistics on the quality of our move ordering.
  cerr << endl;
  double sum = 0;
  for (int i = 0; i < hist_nbuckets; i++)
    sum += hist_pv[i];
  cerr << "pv hist: ";
  for (int i = 0; i < hist_nbuckets; i++)
    cerr << (hist_pv[i] / sum) * 100 << "% ";
  cerr << endl;

  // Display statistics on transposition table hit rate.
  double hit_rate = (double) tt_hits / (tt_hits + tt_misses);
  cerr << "tt hit rate: " << hit_rate * 100 << "%" << endl;
}
 
