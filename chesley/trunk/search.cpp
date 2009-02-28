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
  else
    {
      return false;
    }
}

// Store an entry in the transposition table.
void
Search_Engine::tt_store (uint64 hash, const TT_Entry &in) {
  // Garbage collect the table when it gets very large.
  const uint32 COUNT_MAX = 10 * 1000 * 1000;
  if (tt.size () > COUNT_MAX)
    {
      cerr << "garbage collecting..." << endl;
      int depth = 1;
      do
	{
	  Trans_Table::iterator i;
	  for (i = tt.begin (); i != tt.end (); i++)
	    if (i -> second.depth <= depth) tt.erase (i);
	  depth++;
	} while (tt.size () > COUNT_MAX / 2);

      cerr << "done." << endl;
    }


  // Store an item in the table.
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

  // Clear statistics.
  calls_to_alpha_beta = 0;

#if MTDF
  Move best_move = MTDf (b, 0, 1);
#else
  Move best_move = alpha_beta_with_memory (b, 1);
#endif

  for (int i = 2; i <= depth; i++)
    {
      try
        {
#if MTDF
	  Move best_move = MTDf (b, best_move.score, 1);
#else
	  best_move = alpha_beta_with_memory (b, i);
#endif

        }
      catch (int code)
        {
          assert (code == SEARCH_INTERRUPTED);
	  cerr << "timeout searching ply: " << i << endl;
	  return best_move;
        }
    }

  return best_move;
}

Move
Search_Engine::MTDf (const Board &root, int f, int d) {

  int beta;
  int g = f;
  int upperbound = +INFINITY;
  int lowerbound = -INFINITY;
  Move m;

  do
    {
	{
	  if (g == lowerbound) beta = g + 1; else beta = g;
	  m = alpha_beta_with_memory (root, d, beta - 1, beta);
	  g = m.score;
	  if (g < beta) upperbound = g; else lowerbound = g;
	}
    } while (lowerbound < upperbound);

  return m;
}

/************************************************************************/
/* Search_Engine::alpha_beta_with_memory ()                             */
/*                                                                      */
/* The arguments alpha and beta provide a lower and upper bound on the  */
/* correct value of the top-level search we a conducting, so any        */
/* subtree we can prove has a value outside that range can not possibly */
/* contain the value we're searching for. This allows us to cut of      */
/* branches of the tree and provides an exponential speed up in the     */
/* search.                                                              */
/************************************************************************/

Move
Search_Engine::alpha_beta_with_memory
(const Board &b, int depth, int alpha, int beta) {
  Color player = b.flags.to_move;

  /***********************/
  /* Collect statistics. */
  /***********************/

  calls_to_alpha_beta++;

  /**************************************/
  /* Abort if we have been interrupted. */
  /**************************************/

  if (interrupt_search)
    {
      throw (SEARCH_INTERRUPTED);
    }

  /*****************************************************/
  /* Try to find this node in the transposition table. */
  /*****************************************************/

#if USE_TRANS_TABLE
  TT_Entry entry;
  bool found_tt_entry;

  found_tt_entry = tt_fetch (b.hash, entry);
  if (found_tt_entry && entry.depth >= depth)
    {
      /***************************************************************/
      /* Bail out if the bounds we have previously computed for this */
      /* node tells us the true minimax value isn't in this subtree. */
      /***************************************************************/

      if (entry.lowerbound >= beta)
	{
	  entry.move.score = entry.lowerbound;
	  return entry.move;
	}

      if (entry.upperbound <= alpha)
	{
	  entry.move.score = entry.upperbound;
	  return entry.move;
	}

      /************************************************************/
      /* Otherwise see if the bounds we saved allow us to tighten */
      /* [alpha, beta].                                           */
      /************************************************************/

      alpha = max (alpha, entry.lowerbound);
      beta = min (beta, entry.upperbound);
    }
#endif

  /*********************************************/
  /*  Return heuristic estimate at depth zero. */
  /*********************************************/

  if (depth == 0)
    {
      return Move (eval (b, player));
    }

  /*****************************************************/
  /* Otherwise recurse over the children of this node. */
  /*****************************************************/

  else
    {
      bool fail_high = false;
      bool fail_low = false;
      Move_Vector moves (b);

#if ORDER_MOVES
      // Even this trivial move ordering criterion makes a huge
      // difference.
      insertion_sort <Move_Vector, Move> (moves);
#endif

      // Minimax over children.

#if XXX
      Move best_move = moves[0];
#endif

      Move best_move;
      if (player == WHITE)
	{
	  best_move = Move (-INFINITY);
	}
      else
	{
	  best_move = Move (+INFINITY);
	}

      bool at_least_one_legal_move = false;
      for (int i = 0; i < moves.count; i++)
        {
          Board child (b);
          if (child.apply (moves[i]))
            {
              at_least_one_legal_move = true;

              /***************************/
              /* Maximizing at this node. */
              /***************************/

              if (player == WHITE)
                {
                  if (alpha >= beta)
                    {
                      /*************************************************/
                      /* The value of this position is at least alpha, */
                      /* and since the value we're looking for is no   */
                      /* greater than beta we bail out.                */
                      /*************************************************/

		      best_move.score = alpha;
                      fail_high = true;
                      break;
                    }

		  Move s = alpha_beta_with_memory (child, depth - 1, alpha, beta);

		  if (s.score > best_move.score)
		    {
		      best_move = moves[i];
		      best_move.score = s.score;
		    }

                  if (s.score > alpha)
                    {
                      alpha = s.score;
                    }
                }

              /****************************/
              /* Minimizing at this node. */
              /****************************/

              else
                {

                  if (beta <= alpha)
                    {
                      /***********************************************/
                      /* The value of this position is at most beta, */
                      /* and since the value we're looking for is no */
                      /* less than alpha we bail out.                */
                      /***********************************************/

		      best_move.score = beta;
                      fail_low = true;
                      break;
                    }

		  Move s = alpha_beta_with_memory (child, depth - 1, alpha, beta);

		  if (s.score < best_move.score)
		    {
		      best_move = moves[i];
		      best_move.score = s.score;
		    }

                  if (s.score < beta)
                    {
                      beta = s.score;
                    }

                }
            }
        }

      /**********************************************************/
      /* If we found at least one move, return the best move we */
      /* found.                                                 */
      /**********************************************************/



      if (at_least_one_legal_move)
        {
#if XXX
          if (player == WHITE)
            {
              best_move.score = alpha;
            }
          else
            {
              best_move.score = beta;
            }
#endif
        }


      /*************************************************************/
      /* Otherwize, the game is over and we determine whether it's */
      /* a draw or somebody won.                                   */
      /*************************************************************/

      else
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

              // best_move is a win.
              best_move = Move (mate_val);
            }
          else
            {
              // best_move is a draw.
              best_move = Move (0);
            }
        }

      /*****************************************************************/
      /* If we have no entry for this position, or we have scored this */
      /* position to a deeper depth, then store the results in the     */
      /* transposition table.                                          */
      /*****************************************************************/

#if USE_TRANS_TABLE
      if (!found_tt_entry || depth > entry.depth)
	{
	  entry.move = best_move;
	  entry.depth = depth;

	  // If he have established an upper or lower bound on this
	  // position, update the table.

	  if (fail_low)
	    {
	      entry.upperbound = best_move.score;
	    }

	  else if (fail_high)
	    {
	      entry.lowerbound = best_move.score;
	    }

	  else
	    {
	      entry.upperbound = best_move.score;
	      entry.lowerbound = best_move.score;
	    }

	  tt_store (b.hash, entry);
	}
#endif

      return best_move;
    }
}
