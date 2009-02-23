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
/* Search_Engine::iterative_deepening                                 */
/*                                                                    */
/* Do a depth first search repeatedly, each time increasing the depth */
/* by one. This allows us to return a reasonable move if we are       */
/* interrupted.                                                       */
/**********************************************************************/

Move
Search_Engine::iterative_deepening (const Board &b, int depth) {
  Move best_move = alpha_beta (b, 1);

  for (int i = 2; i < depth; i++)
    {
      try
        {
          best_move = alpha_beta (b, i);
        }
      catch (int code)
        {
          assert (code == SEARCH_INTERRUPTED);
          return best_move;
        }
    }

  return best_move;
}

/***********************************************************************/
/* Search_Engine::alpha_beta ()                                        */
/*                                                                     */
/* The arguments alpha and beta provide a lower and upper bound on the */
/* correct value of the top-level search we a conducting, so any node  */
/* we can prove has a value outside that range can not possibly        */
/* contain the value we're searching for. This allows us to cut of     */
/* branches of the tree and provides an exponential speed up in the    */
/* search.                                                             */
/***********************************************************************/

Move
Search_Engine::alpha_beta (const Board &b, int depth, int alpha, int beta) {
  Color player = b.flags.to_move;

  /**************************************/
  /* Abort if we have been interrupted. */
  /**************************************/

  if (interrupt_search)
    {
      throw (SEARCH_INTERRUPTED);
    }

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
      insertion_sort <Move_Vector, Move> (moves);

      // Minimax over children.
      Move best_move = moves[0];
      bool at_least_one_legal_move = false;
      for (int i = 0; i < moves.count; i++)
        {
          Board child (b);
          if (child.apply (moves[i]))
            {
              at_least_one_legal_move = true;
              Move m = alpha_beta (child, depth - 1, alpha, beta);

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

                  if (alpha >= beta) 
                    {
                      /*************************************************/
                      /* The value of this position is at least alpha, */
                      /* and since the value we're looking for is no   */
                      /* greater than beta we bail out.                */
                      /*************************************************/

                      fail_high = true;
                      break;
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

      return best_move;
    }
}
