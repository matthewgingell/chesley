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
Search_Engine :: choose_move (Board &b, Board::History &h, int depth) {
  interrupt_search = false;
  return iterative_deepening (b, h, depth >= 0 ? depth : max_depth);
}

// Return an estimate of the value of a position.
score_t
Search_Engine :: score (const Board &b, Board::History &h, int depth) {
  interrupt_search = false;
  return (iterative_deepening (b, h, depth >= 0 ? depth : max_depth)).score;
}

// Fetch the principle variation for the most recent search.
void 
Search_Engine :: fetch_pv (const Board &b, Move_Vector &out) {
#if USE_TRANS_TABLE
  Board next = b;
  TT_Entry entry;
  out.clear ();  
  while (tt_fetch (next.hash, entry))
    {
      Board last = next;
      out.push (entry.move);
      next.apply (entry.move);
      if (out.count > 30) break;
    }
#endif // USE_TRANS_TABLE
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
Search_Engine::iterative_deepening (const Board &b, Board::History &h, int depth) {

#if USE_HIST_HEURISTIC
  /***************************************************************/
  /* Age the history table. Old results age exponentially, so    */
  /* hopefully we don't end up with a table full of junk values. */
  /***************************************************************/

  for (int from = 0; from < 64; from++)
    for (int to = 0; to < 64; to++)
      {
 	  hh_white[from][to] /= 2;
	  hh_black[from][to] /= 2;
      }
#endif // USE_HIST_HEURISTIC

  // Always return a search at least to depth one.
  Move best = alpha_beta (b, h, 1);

  // Clear statistics.
  calls_to_alpha_beta = 0;

  /******************************************************************/
  /* Search repeatedly until we are interrupted or hit ply 'depth'. */
  /******************************************************************/

  for (int i = 2; i <= depth; i++)
    {
      try 
	{
	  best = alpha_beta (b, h, i, -INF, +INF);
	} 
      catch (int exp) 
	{
	  assert (exp == SEARCH_INTERRUPTED);
	  cerr << "interrupt: finished searching ply: " << (i - 1) << endl;
	  break;
	}
    }

  return best;
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
(const Board &b, Board::History &h, int depth, score_t alpha, score_t beta) {
  Color player = b.flags.to_move;
  Move best ((player == WHITE ? -INF : +INF));

  /***********************/
  /* Collect statistics. */
  /***********************/

  calls_to_alpha_beta++;

  /**************************************/
  /* Abort if we have been interrupted. */
  /**************************************/

  if (interrupt_search)
    throw (SEARCH_INTERRUPTED);

#if 1
  /* If 50 moves have passed without a capture or a move
     by a pawn, score this child as a draw. */
  if (b.half_move_clock >= 50)
    {
      return Move (0);
    }
  
  /* If this child exceeds the triple repetition rule,
     score it as a draw. */
  else if (h.is_triple_repetition (b))
    {
      return Move (0);
    }
#endif


#if USE_TRANS_TABLE
  /*****************************************************/
  /* Try to find this node in the transposition table. */
  /*****************************************************/

  TT_Entry entry;
  bool found_tt_entry;

  found_tt_entry = tt_fetch (b.hash, entry);
  if (found_tt_entry && entry.depth == depth)
    {
      if (entry.type == TT_Entry::EXACT_VALUE)
	return entry.move;

      else if (entry.type == TT_Entry::LOWERBOUND)
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
      best = Move (eval (b, player));
    }

  /*****************************************************/
  /* Otherwise recurse over the children of this node. */
  /*****************************************************/

  else
    {
      // Generate moves available from this position and sort them
      // from best to worst by their estimated value.
      Move_Vector moves (b);
      order_moves (b, moves);

      /**************************/
      /* Minimax over children. */
      /**************************/

      // Push this position on to the history.
      h.push (b);

      for (int i = 0; i < moves.count; i++)
        {
          Board child (b);
          if (child.apply (moves[i]))
            {
	      score_t val;
	      Move s = alpha_beta (child, h, depth - 1, alpha, beta);
	      val = s.score;

              /****************************/
              /* Maximizing at this node. */
              /****************************/

              if (player == WHITE)
                {
		  if (val > best.score)
		    {
		      best = moves[i];
		      best.score = val;

		      if (val > alpha)
			{
			  alpha = val;
			}

		      if (val >= beta)
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
		  if (val < best.score)
		    {
		      best = moves[i];
		      best.score = val;

		      if (val < beta)
			{
			  beta = val;
			}

		      if (val <= alpha)
			{
			  break;
			}
		    }
                }
            }
        }

      
      // Pop this position from the history.
      h.pop (b);

      // If we couldn't find at least one legal move the game is over.
      if (best.is_null ())
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

              best = Move (mate_val);
            }
          else
            {
              // best is a draw.
              best = Move (0);
            }
        }

#if USE_TRANS_TABLE
      /*********************************************/
      /* Store results in the transposition table. */
      /*********************************************/

      if (!found_tt_entry || depth > entry.depth)
	{
	  entry.depth = depth;
	  entry.move = best;

	  if (best.score <= alpha)
	    entry.type = TT_Entry :: LOWERBOUND;
	  else if (best.score >= beta)
	    entry.type = TT_Entry :: UPPERBOUND;
	  else
	    entry.type = TT_Entry :: EXACT_VALUE;

	  tt_store (b.hash, entry);
	}
#endif // USE_TRANS_TABLE

    }

#if USE_HIST_HEURISTIC
  /*****************************************/
  /* Enter this move in the history table. */ 
  /*****************************************/

  if (player == WHITE) 
    {
      hh_white[best.from][best.to] += 1 << depth;
    }
  else
    {
      hh_black[best.from][best.to] += 1 << depth;
    }
#endif // USE_HIST_HEURISTIC

  return best;
}

// Estimate the value of a move.
inline int value (const Move &m) { 
  return m.score; 
}

// Attempt to order moves to improve our odds of getting earlier
// cutoffs.
void 
Search_Engine::order_moves (const Board &b, Move_Vector &moves) {
  TT_Entry e;
  bool have_entry = tt_fetch (b.hash, e);
  Color player = b.flags.to_move;

  for (int i = 0; i < moves.count; i++)
    {
      assert (moves[i].score == 0);

      /* If we previously computed that moves[i] is the best move from
	 this position, make sure it is searched first. */
      if (have_entry && moves[i] == e.move)
	{
	  moves[i].score = -INF;
	}
#if USE_HIST_HEURISTIC
      else
	{
	  /* Otherwise, score based on it's rate and depth of cutoffs
	     recently. */
	  if (player == WHITE)
	    {
	      moves[i].score = -hh_white [moves[i].from][moves[i].to];
	    }
	  else
	    {
	      moves[i].score = -hh_black [moves[i].from][moves[i].to];
	    }
	}
#endif // USE_HIST_HEURISTIC
    }
  
  insertion_sort <Move_Vector, Move> (moves);
}

/**********************************************************************/
/* Transposition tables                                               */
/*                                                                    */
/* Operations on the transposition table.                             */
/**********************************************************************/

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

  return false;
}

// Store an entry in the transposition table.
inline void
Search_Engine::tt_store (uint64 hash, const TT_Entry &in) {
  const uint32 MAX_COUNT = 25 * 1000 * 1000;
  if (tt.size () >= MAX_COUNT) tt.erase (tt.begin ());
  tt.erase (hash);
  tt.insert (pair <uint64, TT_Entry> (hash, in));
}
