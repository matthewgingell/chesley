/*
  Implementation of the engine's search strategy.

  Matthew Gingell
  gingell@adacore.com
*/

#include <cassert>
#include <iostream>
#include "chesley.hpp"

using namespace std;

/*****************************/
/* Game over exception type. */
/*****************************/

ostream & operator<< (ostream &os, Game_Over e) {
  return os << "Game over with " << e.status;
}

// Select a move. Caller must check the value of b.flags.status after
// this call, since it will be set and an exception raised if no moves
// can be generated from this position.
Move
Search_Engine :: choose_move (Board &b) {

  /***************************/
  /* Find best scoring move. */
  /***************************/

  Move_Vector moves (b);
  Color c = b.flags.to_move;
  int best_score, best_index;

  if (c == WHITE) 
    {
      best_score = -INFINITY;
    }
  else
    {
      best_score = +INFINITY;
    }

  best_index = -1;

  for (int i = 0; i < moves.count; i++)
    {
      Board child (b);
      if (child.apply (moves[i]))
	{
	  int s = score (child, 1, -INFINITY, INFINITY);

	  moves[i].score = s;

	  if ((c == WHITE && s > best_score) ||
	      (c == BLACK && s < best_score))
	    {
	      best_score = s;
	      best_index = i;
	    }
	}
    }

  if (best_index != -1) 
    {
      cerr << moves << endl;
      cerr << best_index << endl;
      cerr << moves[best_index] << endl;
    }

  /********************************************************************/
  /* Handle end of game, raising an exception if there are no legal.  */
  /* moves from this position.                                        */ 
  /********************************************************************/

  // If color has no legal moves, then the game is over.
  if (best_index == -1)
    {
      if (b.in_check (c))
	{
	  if (c == WHITE) 
	    {
	      b.flags.status = GAME_WIN_BLACK;
	    }
	  else 
	    {
	      b.flags.status = GAME_WIN_WHITE;
	    }
	}
      else
	{
	  b.flags.status = GAME_DRAW;
	}
      throw Game_Over (b.flags.status);
    }
  else
    {
      b.flags.status = GAME_IN_PROGRESS;
      moves[best_index].score = best_score;
      return moves[best_index];
    }
}

// Score a move using minimax with alpha-beta pruning.
int 
Search_Engine :: score
(const Board &b, int depth, int alpha, int beta) {

  /***********************/
  /* Collect statistics. */
  /***********************/

  score_count++;

  /********************************************/
  /* Return heuristic value at maximum depth. */
  /*******************************************/

  if (depth == max_depth) 
    {
      int e = eval (b, depth);  
      assert (e > -INFINITY && e < INFINITY);
      return e;
    }

  /***********************************************************/
  /* Recurse with alpha-beta pruning over legal child moves. */
  /***********************************************************/

  int count = 0;
  Color c = b.flags.to_move;

  Move_Vector moves (b);

#if 0
  bubble_sort (moves);
#endif

  for (int i = 0; i < moves.count; i++) 
    {
      Board child (b);
      if (child.apply (moves[i]))
	{
	  // Count legal moves.
	  count++;

	  // Maximizing at node.
	  if (c == WHITE)
	    {
	      alpha = max (alpha, score (child, depth + 1, alpha, beta));

	      // Alpha cut-off
	      if (alpha >= beta) 
		{
		  return beta; 
		}
	    }

	  // Minimizing at this node.
	  else // c == BLACK
	    {
	      beta = min (beta, score (child, depth + 1, alpha, beta));

	      // Beta cut-off
	      if (beta <= alpha) 
		{
		  return alpha;
		}
	    }
	}
    }

  /*******************************************************************/
  /* If we found at least one child, return the best score we found. */
  /*******************************************************************/
  
  if (count > 0) 
    {
      if (c == WHITE)
	{
	  return alpha;
	}
      else
	{
	  return beta;
	}
    }

  /**********************************************************/
  /* Otherwise bottom out since there are no further moves. */
  /**********************************************************/

  if (b.in_check (c))
    {
      if (c == WHITE)
	{
	  return -MATE_VAL + depth;
	}
      else
	{
	  return +MATE_VAL - depth;
	}
    }
  else
    {
      return 0 + c * depth;
    }
}
