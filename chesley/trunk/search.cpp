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

//////////////////////
// Public interface //
//////////////////////

// Compute the principal variation and return its first move.
Move
Search_Engine :: choose_move (Board &b, int depth) {
  Move_Vector pv;
  new_search (b, depth, pv);
  assert (pv.count > 0);
  return pv[0];
}

////////////////////////////
// Private implementation //
////////////////////////////

////////////////////////////////////////////////////////////////////////
// Search_Engine :: new_search ()                                     //
//                                                                    //
// This is the top level entry point to the tree search. It take care //
// of initializing search state, then calls lower level routines to   //
// generate a score and populate the principal variation.             //
////////////////////////////////////////////////////////////////////////

Score
Search_Engine :: new_search 
(const Board &b, int depth, Move_Vector &pv) 
{
  // Clear the history table.
  memset (hh_table, 0, sizeof (hh_table));
  
  // Clear the transposition table.
  tt.clear ();
  
  // Clear statistics.
  calls_to_search = 0;
  calls_to_qsearch = 0;
  memset (move_offsets, 0, sizeof (move_offsets));
  memset (beta_offsets, 0, sizeof (beta_offsets));
  
  return iterative_deepening (b, depth, pv);
}

////////////////////////////////////////////////////////////////////////
// Search_Engine::iterative_deepening                                 //
//                                                                    //
// Do a depth first search repeatedly, each time increasing the depth //
// by one. This allows us to return a reasonable move if we are       //
// interrupted.                                                       //
////////////////////////////////////////////////////////////////////////

Score 
Search_Engine::iterative_deepening 
(const Board &b, int depth, Move_Vector &pv) 
{
  Score s = 0;

  // Print header for posting thinking.
  if (post) 
    {
      cerr << "Move " << b.full_move_clock << ":" << b.half_move_clock << endl;
      cerr << "Ply     Nodes    Qnodes    Time    Eval   Principal Variation" 
           << endl;
    }

  // Search progressively deeper play until we are interrupted.
  for (int i = 1; i <= depth; i++) 
    {
      Move_Vector tmp;
      calls_to_search = 0;
      calls_to_qsearch = 0;
      uint64 start_time = cpu_time ();

#ifdef ENABLE_ASPIRATION_WINDOW
      const int WINDOW = 25;
      const int lower = s - WINDOW;
      const int upper = s + WINDOW;

      // Do an aspiration search.
      if (i > 1) 
        {
          s = search_with_memory  (b, i, 0, tmp, lower, upper);
          
          // Search again with a full window if we fail to find the
          // best move in the aspiration window or if this search
          // failed to return at least one move.
          if (s <= lower || s >= upper || tmp.count == 0)
            {
              tmp.clear ();
              s = search_with_memory (b, i, 0, tmp);
            }
        }
      else
        {
          s = search_with_memory (b, i, 0, tmp);
        }
#else
      s = search_with_memory (b, i, 0, tmp);
#endif /* ENABLE_ASPIRATION_WINDOW */

      if (interrupt_search)
          break;

      // If we got back a principle variation, return it to the caller.
      if (tmp.count > 0)
        {
          pv = tmp;

          // Show thinking.
          if (post)
            {
              double elapsed = ((double) cpu_time () - (double) start_time) / 1000;
              cerr 
                << setw (3)  << i
                << setw (10) << calls_to_search
                << setw (10) << calls_to_qsearch
                << setw (8)  << setiosflags (ios::fixed) << setprecision (2) 
                << elapsed
                << setw (8)  << pv[0].score 
                << "   ";
              Board c = b;
              for (int j = 0; j < pv.count; j++)
                {
                  cerr << c.to_san (pv[j]) << " ";
                  c.apply (pv[j]);
                }
              cerr << endl;
            }
        }
    }

  // Display statistics on the quality of our move ordering.
  cerr << endl;
  double sum = 0;
  for (int i = 0; i < ordering_stats_count; i++)
    sum += move_offsets[i];
  cerr << "move_offsets: ";
  for (int i = 0; i < ordering_stats_count; i++)
    cerr << (move_offsets[i] / sum) * 100 << "% ";
  cerr << endl;

  sum = 0;
  for (int i = 0; i < ordering_stats_count; i++)
    sum += beta_offsets[i];
  cerr << "beta_offsets: ";
  for (int i = 0; i < ordering_stats_count; i++)
    cerr << (beta_offsets[i] / sum) * 100 << "% ";
  cerr << endl;

  // Check that we got at least one move.
  assert (pv.count > 0);
  
  return pv[0].score;
}

/////////////////////////////////////////////////////////////////////////
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
  Score s;

  assert (pv.count == 0);

#ifdef ENABLE_TRANS_TABLE
  ///////////////////////////////////////////////////////
  // Try to find this node in the transposition table. //
  ///////////////////////////////////////////////////////

  TT_Entry entry;
  bool have_exact = false;
  bool found_tt_entry = false;
  found_tt_entry = tt_fetch (b.hash, entry);
  if (found_tt_entry && 
      entry.depth == depth && 
      // Try to deal with the graph history problem:
      b.half_move_clock < 50 && rep_count (b) < 3) 
    {
      if (entry.type == TT_Entry::LOWERBOUND)
        alpha = max (entry.move.score, alpha);
      
      else if (entry.type == TT_Entry::UPPERBOUND)
        beta = min (entry.move.score, beta);

      else if (entry.type == TT_Entry::EXACT_VALUE)
        have_exact = true;
    }

  if (have_exact)
    {
      pv.push (entry.move);
      return entry.move.score;
    }
  else
#endif /* ENABLE_TRANS_TABLE */

  rt_push (b);
  s = search (b, depth, ply, pv, alpha, beta, do_null_move);
  rt_pop (b);

#ifdef ENABLE_TRANS_TABLE
  //////////////////////////////////////////////////////
  // Update the transposition table with this result. //
  //////////////////////////////////////////////////////

  if (pv.count > 0 && (!found_tt_entry || depth > entry.depth))
    {
      entry.move = pv[0];
      entry.depth = depth;

      if (s >= beta) 
        entry.type = TT_Entry :: LOWERBOUND;

      else if (s <= alpha)
        entry.type = TT_Entry :: UPPERBOUND;

      else
        entry.type = TT_Entry :: EXACT_VALUE;

      tt_store (b.hash, entry);
    }
#endif/* ENABLE_TRANS_TABLE */

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
  bool found_move = false;

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
    return 0;

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
      alpha = eval (b) - ply;
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
    if (do_null_move &&
        pop_count (b.occupied) > 15 &&
        !b.in_check ((b.to_move ())))
      {
        Board c = b;
        Move_Vector dummy;
        c.set_color (invert_color (b.to_move ()));

        int val = -search_with_memory 
          (c, depth - R - 1, ply + 1, dummy, -beta, -beta + 1, false);

        if (val >= beta) 
          {
            return val;
          }
      }
#endif /* ENABLE_NULL_MOVE */
        
    Move_Vector moves (b);


#ifdef ENABLE_ORDER_MOVES
    order_moves (b, depth, moves, alpha, beta);
#endif

    ////////////////////////////
    // Minimax over children. //
    ////////////////////////////

    int ext = 0;

    // Extend the search by one ply if we are in check.
    if (b.in_check (b.to_move ()))
      {
	ext += 1;
      }

    int mi = 0;
    bool have_pv_move = false;
    for (mi = 0; mi < moves.count; mi++)
      {
	int cs;
        Board c = b;
        Move_Vector cpv;
        if (c.apply (moves[mi]))
          {
            found_move = true;

#if ENABLE_PVS
            // Perform a principal variation search. Note we always do
            // a full width search on the first and second entries in
            // the move list, since experimentally that's were find
            // about 95% of our moves.
	    if (have_pv_move && mi > 1) 
	      {
		cs = -search_with_memory 
		  (c, depth - 1, ply + 1, cpv, -alpha - 1, -alpha, true);
		if (cs > alpha && cs < beta) 
		  {
		    cpv.clear ();
		    cs = -search_with_memory 
		      (c, depth - 1, ply + 1, cpv, -beta, -alpha, true);
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
            if (cs >= beta)
              {
		beta_offsets[min (mi,  ordering_stats_count)]++;
                break;
              }
#endif // ENABLE_ALPHA_BETA

          }
      }

    move_offsets[min (mi,  ordering_stats_count)]++;

    // If we couldn't find a move that applied, then the game is over.
    if (!found_move)
      {
        if (b.in_check (b.to_move ()))
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
      // Sort our best guess first.
      if (moves[i] == best_guess)
        moves[i].score = +INF;
      
      // Followed by captures.
      moves[i].score += 
        100 * (eval_piece (moves[i].capture (b)) - PAWN_VAL);

      // And finally, recent PV and fail-high moves.
      moves[i].score += 
        hh_table[b.to_move ()][depth][moves[i].from][moves[i].to];
      
      if (depth > 0)
	moves[i].score += 
	  hh_table[b.to_move ()][depth][moves[i].from][moves[i].to] / 10;

    }

  insertion_sort <Move_Vector, Move, less_than> (moves);
}

//////////////////////////////////////////////////////////////////////
// Search_Engine :: see ()                                          //
//                                                                  //
// Static exchange evaluation. This routine plays out a series of   //
// captures in least-valuable attacker order, stopping when all     //
// captures are resolved or a capture is disadvantageous for the    //
// moving side. The goal here is to generate a good estimate of the //
// value of a capture in linear time.                               //
//////////////////////////////////////////////////////////////////////

Score
Search_Engine::see (const Board &b, const Move &m) {
  Score s = eval_piece (m.capture (b));
  Board c = b;
  c.clear_piece (m.from);
  c.clear_piece (m.to);
  c.set_piece (m.get_kind (b), b.to_move (), m.to);
  c.set_color (invert_color (b.to_move ()));
  Move lvc = c.least_valuable_attacker (m.to);
  if (!lvc.is_null ())
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

  if (b.in_check (b.to_move ()) && b.child_count () == 0)
    {
      return -(MATE_VAL - ply);
    }

  ////////////////////////////////////////
  // Do static evaluation at this node. //
  ////////////////////////////////////////

  alpha = max (alpha, eval (b) - ply);

  ////////////////////////////////////////
  // Recurse and minimax over children. // 
  ////////////////////////////////////////

  if (alpha < beta) 
    {
      Board c;
      Move_Vector moves;
      
      b.gen_captures (moves);

      ////////////////////
      // Sort captures. //
      ////////////////////

#ifdef ENABLE_SEE
      for (int i = 0; i < moves.count; i++)  
        {
          moves[i].score = see (b, moves[i]);
        }
#else
      for (int i = 0; i < moves.count; i++)  
        {
          moves[i].score = eval_piece (moves[i].capture (b));
        }
#endif
      insertion_sort <Move_Vector, Move, less_than> (moves);
      
      ////////////////////////////
      // Minimax over captures. //
      ////////////////////////////

      for (int i = 0; i < moves.count; i++)
        {
          // Prune losing captures.
          if (moves[i].score < 0) continue;

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
// Transposition tables                                               //
//                                                                    //
// Operations on the transposition table.                             //
////////////////////////////////////////////////////////////////////////

// Fetch an entry from the transposition table. Returns false if no
// entry is found.
inline bool
Search_Engine::tt_fetch (uint64 hash, TT_Entry &out) {
  Trans_Table::iterator i = tt.find (hash);
  if (i != tt.end ()) 
    {
      out = i -> second;
      return true;
    }
  else
    {
      out = TT_Entry ();
      return false;
    }
}

// Store an entry in the transposition table.
inline void
Search_Engine::tt_store (uint64 hash, const TT_Entry &in) {
  if ((tt.size () > TT_SIZE)) 
    {
      cerr << "Garbage collecting..." << endl;
      tt.clear ();
    }
  tt.erase (hash);
  tt.insert (pair <uint64, TT_Entry> (hash, in));
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
