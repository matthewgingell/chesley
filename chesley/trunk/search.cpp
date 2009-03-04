/*
  Implementation of the engine's search strategy.

  Matthew Gingell
  gingell@adacore.com
*/

#include <cassert>
#include "chesley.hpp"

using namespace std;

/********************/
/* Public interface */
/********************/

// Choose a move, score it, and return it.
Move
Search_Engine :: choose_move (Board &b, int depth) {
  interrupt_search = false;
  return iterative_deepening (b, depth >= 0 ? depth : max_depth);
}

// Return an estimate of the value of a position.
int
Search_Engine :: score (const Board &b, int depth) {
  interrupt_search = false;
  return (iterative_deepening (b, depth >= 0 ? depth : max_depth)).score;
}

/**************************/
/* Private implementation */
/**************************/

const int SEARCH_INTERRUPTED = 0x01;

/**********************************************************************/
/* Transposition tables                                               */
/*                                                                    */
/* Operations on the transposition table.                             */
/**********************************************************************/

// Fetch an entry from the transposition table. Returns false if no
// entry is found.
bool
Search_Engine::tt_fetch (uint64 hash, TT_Entry &out) {
  Trans_Table::iterator i = tt.find (hash);

  if (i != tt.end ())
    {
      out = i -> second;
      return true;
    }

  return false;
}

// Store an entry in the transposition table.
void
Search_Engine::tt_store (uint64 hash, const TT_Entry &in) {
  const uint32 MAX_COUNT = 10 * 1000 * 1000;
  if (tt.size () > MAX_COUNT) tt.erase (tt.begin ());
  tt.erase (hash);
  tt.insert (pair <uint64, TT_Entry> (hash, in));
}

/**********************************************************************/
/* Search_Engine::iterative_deepening                                 */
/*                                                                    */
/* Do a depth first search repeatedly, each time increasing the depth */
/* by one. This allows us to return a reasonable move if we are       */
/* interrupted.                                                       */
/**********************************************************************/

Move
Search_Engine::iterative_deepening (const Board &b, int depth) {
  // Always return a search at least to depth one.
  #if MTDF
  Move best_move = MTDf (b, 0, 1);
  #else
  Move best_move = alpha_beta (b, 1);
  #endif

  // Clear statistics.
  calls_to_alpha_beta = 0;

  // Clear the transposition table. Our replacement policy is to
  // overwrite shallower search values with deeper ones, which will
  // eventually result in a table full of deep search values for old
  // positions we're unlikely to see again. To avoid this problem, we
  // clear the table between searches.
  tt.clear ();

  // Search repeatedly until we are interrupted or hit ply 'depth'.
  for (int i = 2; i <= depth; i++)
    try {
#if MTDF
      best_move = MTDf (b, best_move.score, i);
#else
      best_move = alpha_beta (b, i);
#endif
    } catch (int exp) {
      assert (exp == SEARCH_INTERRUPTED);
      cerr << "interrupt: finished searching ply: " << i - 1 << endl;
      break;
    }

  return best_move;
}

/********************************************************************/
/* Search_Engine::MTDf ()                                           */
/*                                                                  */
/* MTD(f) is a search strategy which makes repeated calls to        */
/* alpha_beta with a window of (beta - 1, beta). On each call it    */
/* established a lower or upper bound, till eventually it converges */
/* on a correct minimax value for this position.                    */
/*                                                                  */
/* For move information on this approach, see:                      */
/*                                                                  */
/* Aske Plaat: MTD(f): A Minimax Algorithm faster than NegaScout    */
/* http://www.cs.vu.nl/~aske/mtdf.htm                               */
/********************************************************************/

Move
Search_Engine::MTDf (const Board &root, int f, int d) {
  Move m;
  int g = f, beta;
  int upperbound = +INFINITY, lowerbound = -INFINITY;
  Move best_move ((root.flags.to_move == WHITE ? -INFINITY : +INFINITY));

  while (1)
    {
      if (g == lowerbound) beta = g + 1; else beta = g;
      m = alpha_beta (root, d, beta - 1, beta);
      g = m.score;
      if (g < beta)
	{
	  // Failed low.
	  upperbound = g;
	  if (root.flags.to_move == BLACK)
	    {
	      if (g < best_move.score) best_move = m;
	    }
	}
      else
	{
	  // Failed high.
	  if (g > best_move.score) best_move = m;
	  if (root.flags.to_move == WHITE)
	    {
	      if (g > best_move.score) best_move = m;
	    }
	  lowerbound = g;
	}

      if (lowerbound >= upperbound) break;
    }

  return best_move;
}

/************************************************************************/
/* Search_Engine::alpha_beta ()                                         */
/*                                                                      */
/* The arguments alpha and beta provide a lower and upper bound on the  */
/* correct value of the top-level search we a conducting, so any        */
/* subtree we can prove has a value outside that range can not possibly */
/* contain the value we're searching for. This allows us to cut off     */
/* branches of the tree and provides an exponential speed up in the     */
/* search.                                                              */
/************************************************************************/

Move
Search_Engine::alpha_beta
(const Board &b, int depth, int alpha, int beta) {
  Color player = b.flags.to_move;
  Move best_move ((player == WHITE ? -INFINITY : +INFINITY));

  /***********************/
  /* Collect statistics. */
  /***********************/

  calls_to_alpha_beta++;

  /**************************************/
  /* Abort if we have been interrupted. */
  /**************************************/

  if (interrupt_search)
    throw (SEARCH_INTERRUPTED);

#if USE_TRANS_TABLE
  /*****************************************************/
  /* Try to find this node in the transposition table. */
  /*****************************************************/

  TT_Entry entry;
  bool found_tt_entry;

  found_tt_entry = tt_fetch (b.hash, entry);
  if (found_tt_entry && entry.depth >= depth)
    {
      if (entry.type == TT_Entry::EXACT_VALUE)
	return entry.move;

      if (entry.type == TT_Entry::LOWERBOUND)
	alpha = max (entry.move.score, alpha);

      else if (entry.type == TT_Entry::UPPERBOUND)
	beta = min (entry.move.score, beta);

      if (alpha >= beta)
	return entry.move;
    }
#endif // USE_TRANS_TABLE

  /*********************************************/
  /*  Return heuristic estimate at depth zero. */
  /*********************************************/

  if (depth == 0)
    {
      best_move = Move (eval (b, player));
    }

  /*****************************************************/
  /* Otherwise recurse over the children of this node. */
  /*****************************************************/

  else
    {
      Move_Vector moves (b);

#if ORDER_MOVES
      // Sort moves on the value of the piece being taken.
      insertion_sort <Move_Vector, Move> (moves);

#if USE_TRANS_TABLE
      // If we found a move in the transposition table, swap it to the
      // front of the list.
      if (found_tt_entry)
	for (int i = 0; i < moves.count; i++)
	  if (moves[i] == entry.move)
	    {
	      swap (moves[0], moves[i]);
	      break;
	    }
#endif

#endif // ORDER_MOVES

      /**************************/
      /* Minimax over children. */
      /**************************/

      for (int i = 0; i < moves.count; i++)
        {
          Board child (b);
          if (child.apply (moves[i]))
            {
	      Move s = alpha_beta (child, depth - 1, alpha, beta);
	      int val = s.score;

              /***************************/
              /* Maximizing at this node. */
              /***************************/

              if (player == WHITE)
                {
		  if (val > best_move.score)
		    {
		      best_move = moves[i];
		      best_move.score = val;

		      if (best_move.score > alpha)
			{
			  alpha = best_move.score;
			}

		      if (best_move.score >= beta)
			{
			  break;
			}
		    }
                }

              /****************************/
              /* Minimizing at this node. */
              /****************************/

              else
                {
		  if (val < best_move.score)
		    {
		      best_move = moves[i];
		      best_move.score = val;

		      if (best_move.score < beta)
			{
			  beta = best_move.score;
			}

		      if (best_move.score <= alpha)
			{
			  break;
			}
		    }
                }
            }
        }

      // If we couldn't find at least one legal move the game is over.
      if (best_move.kind == NULL_KIND)
        {
          if (b.in_check (player))
            {
               /*********************************************************/
               /* We need to provide a bonus to shallower wins over     */
               /* deeper ones, since if we're ambivalent between near   */
               /* and far wins at every ply the search will happily     */
               /* prove it can mate in N forever and never actually get */
               /* there.                                                */
               /*********************************************************/

              int mate_val;
              if (player == WHITE)
                {
                  mate_val = -MATE_VAL + (100 - depth);
                }
              else
                {
                  mate_val = +MATE_VAL - (100 - depth);
                }

              best_move = Move (mate_val);
            }
          else
            {
              // best_move is a draw.
              best_move = Move (0);
            }
        }

#if USE_TRANS_TABLE
      /*********************************************/
      /* Store results in the transposition table. */
      /*********************************************/

      if (!found_tt_entry || entry.depth > depth)
	{
	  entry.move = best_move;
	  entry.depth = depth;

	  if (best_move.score <= alpha)
	    entry.type = TT_Entry :: LOWERBOUND;
	  else if (best_move.score >= beta)
	    entry.type = TT_Entry :: UPPERBOUND;
	  else
	    entry.type = TT_Entry :: EXACT_VALUE;

	  tt_store (b.hash, entry);
	}
#endif // USE_TRANS_TABLE

    }
  return best_move;
}
