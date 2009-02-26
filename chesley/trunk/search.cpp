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

  cout << "fetching: ";

  if (i != tt.end ())
    {
      out = i -> second;

      cout << hash  << " : [" << out.lowerbound << ", " << out.upperbound << ", d=" << out.depth << "]" << endl;

      return true;
    }
  else
    {
      cout << hash << " : not found." << endl;

      return false;
    }
}

// Store an entry in the transposition table.
void
Search_Engine::tt_store (uint64 hash, const TT_Entry &in) {


  cout << "storing " << hash << " for depth " << in.depth << endl;

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
  Move best_move = alpha_beta_with_memory (b, 1);

  // Move best_move = MTDf (b, 0, 1);

  for (int i = 2; i <= depth; i++)
    {
      try
        {
#if 1
          best_move = alpha_beta_with_memory (b, i);
#else
	  best_move = MTDf (b, best_move.score, i);
#endif

        }
      catch (int code)
        {
          assert (code == SEARCH_INTERRUPTED);
	  cerr << "timeout searching ply: " << i << endl;
	  //	  tt.clear ();
	  return best_move;
        }
    }

  //  tt.clear ();
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
  bool found_tt_entry = tt_fetch (b.hash, entry);

  if (found_tt_entry)
    cout << "Found entry at depth " << entry.depth << " while searching " << depth << endl;

  if (found_tt_entry && !(entry.depth >= depth))
    cout << "Ignoring because it's too shallow." << endl;

  if (found_tt_entry && entry.depth >= depth)
    {

      cout << "Using it" << endl;

      /***************************************************************/
      /* Bail out if the bounds we have previously computed for this */
      /* node tells us the true minimax value isn't in this subtree. */
      /***************************************************************/

      cout << "alpha = " << alpha << " beta = " << beta << endl;
      cout << "entry.lowerbound = " << entry.lowerbound << " entry.upperbound = " << entry.upperbound << endl;

      if (entry.lowerbound >= beta)
	{
	  cout << "returning it" << endl;
	  entry.move.score = entry.lowerbound;
	  return entry.move;
	}

      if (entry.upperbound <= alpha)
	{
	  cout << "returning it" << endl;
	  entry.move.score = entry.upperbound;
	  return entry.move;
	}

      /************************************************************/
      /* Otherwise see if the bounds we saved allow us to tighten */
      /* [alpha, beta].                                           */
      /************************************************************/

      cout << "narrowing search" << endl;
      alpha = max (alpha, entry.lowerbound);
      beta = min (beta, entry.upperbound);
    }
#endif

  /*********************************************/
  /*  Return heuristic estimate at depth zero. */
  /*********************************************/

  if (depth == 0)
    {
      return Move (NULL_KIND, 0, 0, NULL_COLOR, NULL_KIND, eval (b, player));
    }

  /*****************************************************/
  /* Otherwise recurse over the children of this node. */
  /*****************************************************/

  else
    {
      bool fail_high = false;
      bool fail_low = false;
      Move_Vector moves (b);

      // Even this trivial move ordering criterion makes a huge
      // difference.
#if ORDER_MOVES
      insertion_sort <Move_Vector, Move> (moves);
#endif







      // Minimax over children.
      Move best_move = moves[0];
      bool at_least_one_legal_move = false;
      for (int i = 0; i < moves.count; i++)
        {
          Board child (b);
          if (child.apply (moves[i]))
            {
              at_least_one_legal_move = true;
              Move m = alpha_beta_with_memory (child, depth - 1, alpha, beta);

              /***************************/
              /* Maximizing at this node. */
              /***************************/

              if (player == WHITE)
                {
                  if (m.score > alpha)
                    {
                      alpha = m.score;
                      best_move = moves[i];
                    }
                }

              /****************************/
              /* Minimizing at this node. */
              /****************************/

              else
                {
                  if (m.score < beta)
                    {
                      beta = m.score;
                      best_move = moves[i];
                    }

                  if (beta <= alpha)
                    {
                      /***********************************************/
                      /* The value of this position is at most beta, */
                      /* and since the value we're looking for is no */
                      /* less than alpha we bail out.                */
                      /***********************************************/

                      fail_low = true;
                      break;
                    }
                }
            }
        }
#endif

      /**********************************************************/
      /* If we found at least one move, return the best move we */
      /* found.                                                 */
      /**********************************************************/

      if (at_least_one_legal_move)
        {
          if (player == WHITE)
            {
              best_move.score = alpha;
            }
          else
            {
              best_move.score = beta;
            }
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
              best_move = Move (NULL_KIND, 0, 0, NULL_COLOR, NULL_KIND, mate_val);
            }
          else
            {
              // best_move is a draw.
              best_move = Move (NULL_KIND, 0, 0, NULL_COLOR, NULL_KIND, 0);
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
