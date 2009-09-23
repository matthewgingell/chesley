///////////////////////////////////////////////////////////////////////////////
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

#include <stdlib.h>
#include <cassert>
#include <iomanip>
#include <iostream>

#include "chesley.hpp"

using namespace std;

// Reference to the pawn hash table.
extern PHash ph;

// Exceptions.
const int SEARCH_INTERRUPTED = 1;

// Utility functions.
bool is_mate (Score s) { 
  return abs (s) > MATE_VAL - MAX_DEPTH;
}

// Compute the principal variation and return its first move.
Move
Search_Engine :: choose_move (const Board &b, int depth)
{
  Move_Vector pv;
  assert (!is_triple_rep (b));
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
  // Age the history tables.
  hh_max /= 4;
  mates_max /= 4;
  for (int i = 0; i < 64; i++)
    for (int j = 0; j < 64; j++)
      {
        hh_table[i][j] /= 4;
        mates_table[i][j] /= 4;
      }

  // Age the killers tables.
  for (int i = 0; i < MAX_PLY - 2; i++)
    {
      killers[i] = killers[i + 2];
      killers2[i] = killers[i + 2];
      mate_killer[i] = mate_killer[i + 2];
    }

  for (int i = MAX_PLY - 2; i < MAX_PLY; i++) 
    {
      killers[i] = NULL_MOVE;
      killers2[i] = NULL_MOVE;
      mate_killer[i] = NULL_MOVE;
    }

  // Clear statistics.
  clear_statistics ();

  // Setup the clock.
  new_deadline ();

  // If one has been configured, set a fixed maximum search depth.
  if (controls.fixed_depth > 0) depth = min (depth, controls.fixed_depth);

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
  bool found_mate = false;
  Score best_mate = 0;

  Rep_Table original (rt);
  
  // Print header for post thinking.
  if (post) post_before (b);

  // Search progressively deeper ply until we are interrupted.
  for (int i = 1; i <= depth; i++)
    {
      Move_Vector pv_tmp;
      Score s_tmp = 0;

      // Initialize statistics for this iteration.
      stats.calls_to_search = stats.calls_to_qsearch = 0;
      start_time = mclock ();

      // Do a root search at this position.
      try 
        {
          s_tmp = root_search (b, i, pv_tmp, s);
        } 
      catch (int e)  
        {
          if (e == SEARCH_INTERRUPTED) break;
        }

      // Copy back the score and PV to the caller.
      pv = pv_tmp;
      s = s_tmp;

      // Collect statistics.
      stats.calls_for_depth[i] = stats.calls_to_search + stats.calls_to_qsearch;
      stats.time_for_depth[i] = mclock () - start_time;

      // Because of techniques like grafting the results of searches
      // from the transposition table to shallow searches, etc., we
      // never know if we've found the quickest mate possible. Here we
      // keep looking for better mates until time expires.
      if (found_mate && abs (s) <= best_mate)
        {
          continue;
        }

      if (is_mate (s))
        {
          found_mate = true;
          best_mate = max (abs (s), abs (best_mate));
        }
      
      // Otherwise copy the principle variation back to the caller. If
      // no move had been returned then there is a serious bug.
      if (pv_tmp.count == 0)
        {
          cerr << b.to_fen () << endl;
          cerr << stats.calls_to_search << endl;
          cerr << stats.calls_to_qsearch << endl;
          cerr << i << endl;
          cerr << s << endl;
          cerr << s_tmp << endl;
          cerr << mclock () << endl;
          cerr << controls.interrupt_search << endl;
          cerr << controls.deadline << endl;
          cerr << found_mate << endl;
          assert (0);
        }

      if (post) post_each (b, i, s, pv);
    }

  // Write out statistics about this search.
  if (post) post_after ();

  // Check that we got at least one move.
  assert (pv.count > 0);

  rt = original;
  return s;
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Search_Engine :: root_search ()                                      //
//                                                                      //
// Searches at the root are treated slightly differently, as we always  //
// want a move and never a cutoff.                                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

Score Search_Engine :: 
root_search (const Board &b, int depth, Move_Vector &pv, Score guess)
{
  Move_Vector cpv;
  Score cs;

  // Update statistics.
  stats.calls_to_search++;

#ifdef ENABLE_ASPIRATION_WINDOW
  // Try an aspiration search.
  const int ASPIRATION_WINDOW = 30;
  Score lower = guess - ASPIRATION_WINDOW / 2;
  Score upper = guess + ASPIRATION_WINDOW / 2;
  cs = search_with_memory (b, depth, 0, pv, lower, upper);
  if (cs > lower && cs < upper && pv.count > 0) 
    {
      stats.asp_hits++;
      tt.set (b, EXACT_VALUE, pv[0], cs, depth);
      return cs;
    }
#endif // ENABLE_ASPIRATION_WINDOW
  
  // Generate moves.
  Move_Vector moves (b);
  order_moves (b, 0, moves);
  
  // Recurse over moves.
  int i;
  Score alpha = -INF, beta = INF;
  for (i = 0; i < moves.count; i++)
    {
      int ext;
      Board c = b;
      cpv.clear();

      // Skip this move if it's illegal.
      if (!c.apply (moves[i])) continue;

      // Decide on a depth adjustment for this search.
      ext = depth_adjustment (b, moves[i]);
                            
      // Do principal variation search.
      if (i > 0)
        {
          cs = -search_with_memory 
            (c, depth - 1 + ext, 1, cpv, -alpha - 1, -alpha);
          if (cs <= alpha) continue; 
          else cpv.clear ();
        }
 
      // Recurse with Negamax.
      cs = -search_with_memory 
        (c, depth - 1 + ext, 1, cpv, -beta, -alpha);       
      if (cs > alpha)
        {
          alpha = cs;
          pv = Move_Vector (moves[i], cpv);
          collect_move (depth, 0, moves[i], alpha);
        }
    }
  
  // Store this result in the transposition table.
  tt.set (b, EXACT_VALUE, pv[0], alpha, depth);

  return alpha;
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Search_Engine :: search_with_memory ()                               //
//                                                                      //
// This routine wraps search and memoizes results in the                //
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
  Move m = NULL_MOVE;
  Score s;

  // Update statistics.
  stats.calls_to_search++;

  // Check the clock.
  poll ();
  
  // Try to find this node in the transposition table.
  if (tt_try (b, depth, ply, m, s, alpha, beta))
    {
      if (m != NULL_MOVE) pv.push (m);
      return s;
    }
  
  // See if we can fail high immediately.
  if (alpha >= beta) return alpha;
  
  // Push this position onto the repetition stack and recurse.
  rt_push (b);
  s = search (b, depth, ply, pv, alpha, beta, do_null_move);
  rt_pop (b);
  
  // Update the transposition table and history counters.
  tt_update (b, depth, ply, pv, s, alpha, beta);
  if (pv.count > 0) collect_move (depth, ply, pv[0], s);
  
  return s;
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Search_Engine :: search ()                                           //
//                                                                      //
// This is the negamax implementation at the core of the search         //
// hierarchy.                                                           //
//                                                                      //
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
  int legal_move_count = 0;
  bool in_check = b.in_check ((b.to_move ()));

  // Check 50 move and triple repetition rules.
  if (b.half_move_clock == 100 || is_triple_rep (b)) 
    return 0;

  // Mate distance pruning.
  alpha = max (alpha, (Score) (-MATE_VAL + ply));
  beta = min (beta, (Score) (MATE_VAL - ply));
  if (alpha >= beta) return alpha;
  
  // Return the result of a quiescence search at depth 0.
  if (depth <= 0)
    {
#ifdef ENABLE_QSEARCH
      alpha = qsearch (b, -1, ply, alpha, beta);
#else
      alpha = Eval (b).score ();
#endif // ENABLE_QSEARCH
    }

  // Otherwise recurse over the children of this node.
  else {

#ifdef ENABLE_NULL_MOVE
    /////////////////////////
    // Null move heuristic //
    /////////////////////////

    const int R = depth >= 6 ? 3 : 2;

    // Since we don't have Zugzwang detection we just disable null
    // move if there are fewer than 12 pieces on the board.
    if (do_null_move && !in_check && pop_count (b.occupied) >= 12)
      {
        Board c = b;
        Move_Vector dummy;
        c.set_color (~c.to_move ());
        int val = -search_with_memory
          (c, depth - R - 1, ply, dummy, -beta, -beta + 1, false);
        if (val >= beta) 
          { 
            stats.null_count++;
            return val;
          }
      }
#endif // ENABLE_NULL_MOVE
    
    ///////////////////////////
    // Minimax over children //
    ///////////////////////////

    Move_Vector moves (b);
    order_moves (b, ply, moves);
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

        // Determine the estimated evaluation for this move.
        Score estimate = 
          Eval (b).net_material () + see (b, moves[mi]);

        // Determine whether this moves checks.
        bool c_in_check = c.in_check (c.to_move());

        // Decide on a depth adjustment for this search.
        int ext = depth_adjustment (b, moves[mi]);

#ifdef ENABLE_FUTILITY
        // The approach taken to futility pruning here come from Ernst
        // A. Heinz and his discussion of pruning in Deep Thought at
        // http://people.csail.mit.edu/heinz/dt.

        const int PRE_PRE_FRONTIER = 3;
        const int PRE_FRONTIER = 2;
        const int FRONTIER = 1;

        Score upperbound;
        if (ext == 0 && 
            !in_check && !c_in_check && !have_pv_move && 
            moves[mi].promote == NULL_KIND)
          {
            ////////////////////////
            // Futility pruning.  //
            ////////////////////////

            // At frontier nodes we estimate the most this move
            // could reasonably improve the score of the position,
            // and if it still isn't better than alpha we skip it.
            const Score FUTILITY_MARGIN = 3 * PAWN_VAL;
            upperbound = estimate + FUTILITY_MARGIN;
            if (depth == FRONTIER && upperbound < alpha)
              {
                stats.futility_count++;
                continue;
              }

            ///////////////////////////////
            // Extended futility pruning //
            ///////////////////////////////

            // This is exactly like normal futility pruning, just
            // at pre-frontier nodes with a bigger margin.
            const Score EXT_FUTILITY_MARGIN = 5 * PAWN_VAL;
            upperbound = estimate + EXT_FUTILITY_MARGIN;
            if (depth == PRE_FRONTIER && upperbound < alpha)
              {
                stats.ext_futility_count++;
                continue;
              }

            ////////////////////////////////////////
            // Razoring at pre-pre frontier nodes //
            ////////////////////////////////////////
            
            // If this position looks extremely bad at depth three,
            // proceed with a reduced depth search.
            const Score RAZORING_MARGIN = 8 * PAWN_VAL;
            upperbound = estimate + RAZORING_MARGIN;
            if (depth == PRE_PRE_FRONTIER && upperbound <= alpha)
              {
                stats.razor_count++;
                depth = PRE_FRONTIER;
              }
          }
#endif // ENABLE_FUTILITY

#ifdef ENABLE_PVS

        ////////////////////////////////
        // Principle variation search //
        ////////////////////////////////
        
        if (mi > 0)
          {
            //////////////////////////
            // Late move reductions //
            //////////////////////////

            const int Full_Depth_Count = 4;
            const int Reduction_Limit = 3;

            if (mi >= Full_Depth_Count && depth >= Reduction_Limit &&
                moves[mi].get_promote () != QUEEN && 
                ext == 0 && !in_check && !c_in_check)
              {
                cs = -search_with_memory
                  (c, depth - 2, ply + 1, cpv, -alpha - 1, -alpha, true);
                
                if (cs > alpha) 
                  {
                    cpv.clear ();
                    cs = -search_with_memory
                      (c, depth - 1, ply + 1, cpv, -alpha - 1, -alpha, true);
                  }
                else
                  {
                    stats.lmr_count++;
                    continue;
                  }
              }
            else
              {
                cs = -search_with_memory
                  (c, depth - 1 + ext, ply + 1, cpv, -alpha - 1, -alpha, true);
              }
            
            if (cs > alpha && cs < beta)
              {
                cpv.clear ();
                cs = -search_with_memory
                  (c, depth - 1 + ext, ply + 1, cpv, -beta, -alpha, true);
              }
          }
        else
          {
            cs = -search_with_memory
              (c, depth - 1 + ext, ply + 1, cpv, -beta, -alpha, true);
          }
#else
        cs = -search_with_memory
          (c, depth - 1 + ext, ply + 1, cpv, -beta, -alpha, true);
#endif // ENABLE_PVS
        
        // Test whether this move is better than the moves we have
        // previously analyzed.
        if (cs > alpha)
          {
            alpha = cs;
            if (alpha < beta) 
              {
                have_pv_move = true;
                pv = Move_Vector (moves[mi], cpv);
              }
            else
              {
                collect_fail_high (ply, moves[mi], cs, mi);
                pv = Move_Vector (moves[mi], cpv);
                break;
              }
          }
      }

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
        if (pv.count > 0 && depth <= MAX_DEPTH)
          {
            // Collect statistics.
            stats.hist_pv[min (mi, hist_nbuckets - 1)]++;
          }
      }
  }
  
  return alpha;
}

// Update the corresponding tables when a move fails high.
void 
Search_Engine::collect_fail_high 
(int ply, const Move &m, Score s, int mi) 
{
  // Update the mate killer move.
  if (mi > 0 && is_mate (s) && s > 0 && mate_killer[ply] != m)
    {
      mate_killer[ply] = m;
    }
  else
    {
      // Update the killer moves at this ply.
      if (mi > 0 && 
          m.get_capture () == NULL_KIND && 
          m.get_promote () != QUEEN &&
          killers[ply] != m)
        {
          killers2[ply] = killers[ply];
          killers[ply] = m;
          assert (killers[ply] != killers2[ply]);
        }
    }
}

// Update the corresponding tables when a search finds a move.
void 
Search_Engine::collect_move (int depth, int ply, const Move &m, Score s) {
  // Update the history table.
  uint64 val = hh_table [m.from][m.to] += 1 << depth;
  hh_max = max (val, hh_max);

  // Update the mates table.
  bool mate = (s > 0 && is_mate (s));
  if (mate) 
    {
      uint64 val = mates_table [m.from][m.to] -= 1 << ply;
      mates_max = max (val, mates_max);
    }
}

// Attempt to order moves to improve our odds of getting earlier
// cutoffs.
void
Search_Engine :: order_moves
(const Board &b, int ply, Move_Vector &moves) {
  int32 scores[moves.count];
  ZERO (scores);
  Move best_guess = tt_move (b);

  ///////////////////////////////////////////////////////////////////
  // The general approach here follows Ed Schroder's discussion at //
  // http://members.home.nl/matador/chess840.htm                   //
  ///////////////////////////////////////////////////////////////////

  const int32 HASH_MOVE =       150 * 1000;
  const int32 MATE_KILLER =     125 * 1000;
  const int32 WINNING_CAPTURE = 100 * 1000;
  const int32 QUEEN_PROMOTION =  75 * 1000;
  const int32 EVEN_CAPTURE    =  50 * 1000;
  const int32 KILLER_1        =  25 * 1000;
  const int32 KILLER_2        =  10 * 1000;
  const int32 CASTLING        =   9 * 1000;

  // Score each move.
  for (int i = 0; i < moves.count; i++)
    {
      const Move m = moves[i];

      // Sort the best guess move first.
      if (m == best_guess) 
        {
          scores[i] = HASH_MOVE;
          continue;
        }

      // Apply mate move bonus.
      if (m == mate_killer[ply]) 
        {     
          scores[i] += MATE_KILLER;
          continue;
        }

      if (ply > 2 && m == mate_killer[ply - 2]) 
        {     
          scores[i] += MATE_KILLER - 1;
          continue;
        }

      // Evaluate captures.
      if (m.get_capture () != NULL_KIND)
        {
          Score sval = see (b, m);
          
          if (sval > 0) 
            { 
              scores[i] = WINNING_CAPTURE + sval;
              continue;
            }
          else if (sval == 0) 
            {
              scores[i] = EVEN_CAPTURE + sval;
              continue;
            } 
          else
            {
              scores[i] = sval;
            }
        }
      
      // Apply promotion bonus.
      if (m.get_promote () == QUEEN && 
          m.get_capture () == NULL_KIND) 
        {
          scores[i] += QUEEN_PROMOTION;
          continue;
        }

      // Apply killer move bonus.
      if (m == killers[ply]) 
        {
          scores[i] += KILLER_1;
          continue;
        }

      if (ply > 2 && m == killers[ply - 2]) 
        {
          scores[i] += KILLER_1 - 1;
          continue;
        }

      if (m == killers2[ply]) 
        {
          scores[i] += KILLER_2;
          continue;
        }

      if (ply > 2 && m == killers2[ply - 2]) 
        {
          scores[i] += KILLER_2 - 1;
          continue;
        }

      // Apply castling bonus.
      if (moves[i].is_castle ())
        {
          scores[i] += CASTLING;
          continue;
        }

      // Apply mate bonus.
      uint64 mval = mates_table[m.from][m.to];
      if (mates_max) scores[i] += (ROOK_VAL * mval) / mates_max;
      
      // Apply history bonus.
      uint64 hval = hh_table[m.from][m.to];
      if (hh_max) scores[i] += (PAWN_VAL * hval) / hh_max;
      
      // Apply piece square table bonus.
      scores[i] += Eval::psq_value (b, m);
    }

  moves.sort (scores);  
}

// Return a depth adjustment for a position.
int Search_Engine::depth_adjustment (const Board &b, Move m) {
#ifdef ENABLE_EXTENSIONS
  int ext = 0;

  // Check extension.
  if (b.in_check (b.to_move ())) 
    {
      ext++;
    }

  // Pawn to seventh rank extensions.
  else 
    {
      int rank = idx_to_rank (m.to);
      if ((rank == 1 || rank == 6) && m.get_kind () == PAWN)
        ext += 1;
    }
  
  stats.ext_count += ext;
  return ext;
#else
  return 0;
#endif // ENABLE_EXTENSIONS
}
  
//////////////////////////////////////////////////////////////////////
//                                                                  //
// Search_Engine :: see ()                                          //
//                                                                  //
// Static exchange evaluation. This routine plays out a series of   //
// captures in least-valuable attacker order, stopping when all     //
// captures are resolved or a capture is disadvantageous for the    //
// moving side. The goal here is to generate a good estimate of the //
// value of a capture in linear time. (Robert Hyatt refers to this  //
// as "minimaxing a unary tree.")                                   //
//                                                                  //
//////////////////////////////////////////////////////////////////////

Score
Search_Engine :: see (const Board &b, const Move &m) const {
#ifdef ENABLE_SEE
  if (m.get_capture () == NULL_KIND) return 0;
  Score s = Eval::eval_victim (m);

  // Construct child position.
  Board c = b;
  c.clear_piece (m.get_kind (), b.to_move (), m.from);
  c.clear_piece (m.get_capture (), ~b.to_move (), m.to);
  c.set_piece (m.get_kind (), b.to_move (), m.to);
  c.set_color (invert (b.to_move ()));

  Move lvc = c.least_valuable_attacker (m.to);
  if (lvc != NULL_MOVE)
    {
      assert (lvc.to == m.to);
      s -= max (see (c, lvc), Score (0));
    }

  return s;
#else 
  return Eval::eval_capture (m);
#endif // ENABLE_SEE
}

// Quiescence search.
Score
Search_Engine :: qsearch
(const Board &b, int depth, int ply,
 Score alpha, Score beta)
{
  Move_Vector dummy;

  // Check the clock.
  poll ();

  // Update statistics.
  stats.calls_to_qsearch++;

  // Do static evaluation at this node.
  Score static_eval = Eval (b).score ();

#if 0
  // Delta pruning.
  if (static_eval + QUEEN_VAL < alpha)
    {
      stats.delta_count++;
      return alpha;
    }
#endif

  // Decide whether to stand pat.
  if (static_eval > alpha)
    {
      alpha = static_eval;
    }

  // Recurse and minimax over children.
  if (alpha < beta)
    {
      Board c;
      Move_Vector moves;

      ////////////////////////////////
      // Generate and sort captures //
      ////////////////////////////////

      b.gen_captures (moves);
      b.gen_promotions (moves);

      if (moves.count > 0)
        {
          int32 scores [moves.count];
          ZERO (scores);
          for (int i = 0; i < moves.count; i++) 
            {
              scores[i] = see (b, moves[i]);
              if (moves[i].get_promote () == QUEEN) 
                scores[i] += QUEEN_VAL - PAWN_VAL;
            }
          moves.sort (scores);

          // Minimax over captures.
          int mi = 0;
          for (mi = 0; mi < moves.count; mi++)
            {
              c = b;
              if (c.apply (moves[mi]))
                {
                  alpha = max
                    (alpha, 
                     Score (-qsearch (c, depth - 1, ply + 1, -beta, -alpha)));
                  if (alpha >= beta)
                    break;
                }
            }

          // Collect statistics.
          stats.hist_qpv[min (mi, hist_nbuckets - 1)]++;
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
(const Board &b, int32 depth, int32 ply,
 Move &m, Score &s, Score &alpha, Score &beta)
{
#if ENABLE_TRANS_TABLE
  SKind hash_skind = NULL_SKIND;
  Move hash_move = NULL_MOVE;
  Score hash_score = 0;
  int hash_depth = 0;

  // Find the hash table entry for this position.
  hash_skind = tt.lookup (b, hash_move, hash_score, hash_depth);

  if (hash_skind == NULL_SKIND)
    {
      return false;
    }
  else
    {
      if (hash_depth >= depth &&
          b.half_move_clock < 90 && 
          rep_count (b) == 0)
        {
          // Mate scores need to be treated specially when fetched
          // from the cache.
          m = hash_move;
          if (is_mate (hash_score))
            hash_score -= sign (hash_score) * ply;
          
          if (hash_skind == LOWER_BOUND)
            {
              alpha = max (hash_score, alpha);
              return false;
            }

          else if (hash_skind == UPPER_BOUND)
            {
              beta = min (hash_score, beta);
              return false;
            }

          else if (hash_skind == EXACT_VALUE)
            {
              s = hash_score;
              return true;
            }
        }
    }
#endif

  return false;
}

// Fetch the move, if any, associated with this position in the cache.
Move 
Search_Engine :: tt_move (const Board &b) {
#if ENABLE_TRANS_TABLE  
  return tt.get_move (b);
#else
  return NULL_MOVE;
#endif // ENABLE_TRANS_TABLE
}

void
Search_Engine :: tt_update
(const Board &b, int32 depth, int32 ply, 
 const Move_Vector &pv, Score s, int32 alpha, int32 beta)
{
#if ENABLE_TRANS_TABLE
  assert (alpha <= beta);

  // This rule seems to work well in practice.
  if (pv.count == 0 && !tt.free_entry (b)) return;

  // Determine the kind of value we are recording.
  SKind skind;
  if (s >= beta)  skind = LOWER_BOUND;
  else if (s <= alpha) skind = UPPER_BOUND;
  else skind = EXACT_VALUE;
  
  // The case of putting a mate score in the cache has to be treated
  // specially.
  if (is_mate (s))
    {
      int mate_ply_from_root = MATE_VAL - abs (s);
      int mate_ply_from_here = mate_ply_from_root - ply;
      s = sign (s) * (MATE_VAL - mate_ply_from_here);
    }

  Move m = (pv.count > 0) ? pv[0] : NULL_MOVE;
  
#if 0 
  Move old_move;
  Score old_score;
  int old_depth;
  tt.lookup (b, old_move, old_score, old_depth);
#endif

#if 0
  if (depth > old_depth) 
#endif

    // Update the entry.
    tt.set (b, skind, m, s, depth);

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
  assert (rt.size () < 1000);
  Rep_Table :: iterator i = rt.find (b.hash);
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

///////////////////////////////////////////////
// Time control.                             //
//                                           //
// Routines for managing game clock resource //
///////////////////////////////////////////////

// Setup a deadline for this search.
void
Search_Engine :: new_deadline ()
{
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
          controls.deadline = mclock () +
            (controls.time_remaining) / (controls.moves_remaining + 5);
        }
      else
        {
          // If time is limited but the move count is not, always
          // assume the game will end in 25 more moves.
          controls.deadline = 
            mclock () + (controls.time_remaining - 1000) / 25;
        }
    }
  else
    {
      // Some reasonable time control should always be configured.
      assert (0);
    }
}

// Method called periodically to implement time control.
inline void
Search_Engine :: poll () {
  if ((stats.calls_to_qsearch + stats.calls_to_search) % 64 * 1024 == 0)
    {
      uint64 clock = mclock ();
      if (controls.deadline > 0 && clock >= controls.deadline)
        {
          controls.interrupt_search = true;
          throw SEARCH_INTERRUPTED;
        }
    }
}

// Set fixed depth per move.
void
Search_Engine :: set_fixed_depth (int depth) {
  controls.fixed_depth = depth;
  return;
}

// Set fixed time per move in milliseconds.
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

// Write thinking output before iterative deepening starts.
void
Search_Engine :: post_before (const Board &b) {
  cout << (b.to_fen ()) << endl;
  if (Session::protocol == XBOARD)
    {
      cout << "Ply    Eval    Time     Nodes   Principal Variation";
    }
  else
    {
      cout << "Ply    Eval    Time     Nodes    QNodes   Principal Variation";
    }
  cout << endl;
}

// Write thinking for each depth during iterative deepening.
void
Search_Engine :: 
post_each (const Board &b, int depth, Score s, const Move_Vector &pv) {
  double elapsed = ((double) mclock () - (double) start_time) / 1000;
  Board c = b;

  // Xboard format is: ply, score, time, nodes, pv.

  cout << setiosflags (ios :: right);

  // Ply.
  cout << setw (3) << depth;

  // Score.
  if (is_mate (s))
    {
      cout << setw (2) << (s > 0 ? "+" : "-");
      cout << setw (4) << "Mate";
      cout << setw (2) << (int) (MATE_VAL - abs(s));
    }
  else
    {
      cout << setw (8) << s;
    }

  // Time elapsed. Xboard expects time in centiseconds.
  if (Session::protocol == XBOARD)
    {
      cout << setw (8) << setiosflags (ios :: fixed) 
           << setprecision (0) << elapsed * 100;
    }
  else
    {
      cout << setw (8) << setiosflags (ios :: fixed) 
           << setprecision (2) << elapsed;
    }

  // Node count.
  if (Session::protocol == XBOARD) 
    {
      cout << setw (10);
      cout << stats.calls_to_search + stats.calls_to_qsearch;
    }
  else
    {
      cout << setw (10);      
      cout << stats.calls_to_search;
      cout << setw (10);
      cout << stats.calls_to_qsearch;
    }
  cout << "   ";
  
  // Principle variation.
  Move_Vector pve = pv;
  //  tt_extend_pv (b, pve);
  for (int i = 0; i < pve.count; i++)
    {
      cout << c.to_san (pve[i]) << " ";
      c.apply (pve[i]);
    }

  cout << endl;
}

// Write thinking output after iterative deepening ends.
void
Search_Engine :: post_after () {
  // Display statistics on the quality of our move ordering.
  cout << endl;
  double sum = 0;
  for (int i = 0; i < hist_nbuckets; i++)
    sum += stats.hist_pv[i];
  cout << "pv hist: ";
  for (int i = 0; i < hist_nbuckets; i++)
    cout << setprecision (2) << (stats.hist_pv[i] / sum) * 100 << "% ";
  cout << endl;

  sum = 0;
  for (int i = 0; i < hist_nbuckets; i++)
    sum += stats.hist_qpv[i];
  cout << "qpv hist: ";
  for (int i = 0; i < hist_nbuckets; i++)
    cout << setprecision (2) <<  (stats.hist_qpv[i] / sum) * 100 << "% ";
  cout << endl;

  // Display statistics on transposition table hit rate.
  double hit_rate = (double) tt.hits / (tt.hits + tt.misses);
  cout << "tt hit rate " << hit_rate * 100 << "%, ";
  double coll_rate = (double) tt.collisions / tt.writes;
  cout << "coll rate " << coll_rate * 100 << "%, ";

  hit_rate = (double) ph.hits / (ph.hits + ph.misses);
  cout << "ph hit " << hit_rate * 100 << "%, ";
  coll_rate = (double) ph.collisions / ph.writes;
  cout << "ph coll " << coll_rate * 100 << "%, ";

  // Display performance of heuristics.
  cout << "asp: "   << stats.asp_hits << endl;
  cout << "null: "  << stats.null_count;
  cout << ", ext: " << stats.ext_count;
  cout << " rzr: "  << stats.razor_count;
  cout << ", fut: " << stats.futility_count;
  cout << ", xft: " << stats.ext_futility_count;
  cout << ", lmr: " << stats.lmr_count << endl;
  cout << "dlt: "   << stats.delta_count;

  // Display nodes per second.
  uint64 total_nodes = 0;
  uint64 total_time = 0;
  for (int i = 0; i < MAX_DEPTH; i++) {
    total_nodes += stats.calls_for_depth[i];
    total_time += stats.time_for_depth[i];
  }

  if (total_time > 0) 
    {
      cout << setprecision (2) << ", " <<
        (float) total_nodes / (float) total_time << " knps." << endl;
    }
  else
    {
      cout << "? knps." << endl;
    }
}
