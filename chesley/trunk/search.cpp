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

// Compute the principal variation and return it's first move.
Move
Search_Engine :: choose_move (Board &b, int depth) {
  Move_Vector pv;
  new_search (b, depth, pv);
  return pv[0];
}

/**************************/
/* Private implementation */
/**************************/

const int SEARCH_INTERRUPTED = 0x01;

/**********************************************************************/
/* Search_Engine :: new_search ()                                     */
/*                                                                    */
/* This is the top level entry point to the tree search. It take care */
/* of initializing search state, then calls lower level routines to   */
/* generate a score and populate the principal variation.             */
/**********************************************************************/

Score
Search_Engine :: new_search 
(const Board &b, int depth, Move_Vector &pv) 
{
  // Age the history table. Old results age exponentially, so
  // hopefully we don't end up with a table full of junk values.
  for (int color = 0; color < 2; color++)
    for (int d = 0; d < 2; d++)
      for (int from = 0; from < 64; from++)
	for (int to = 0; to < 64; to++)
	  hh_table[b.flags.to_move][depth][from][to] = 0;
  
  // Clear statistics.
  calls_to_alpha_beta = 0;

  return iterative_deepening (b, depth, pv);
}

/**********************************************************************/
/* Search_Engine::iterative_deepening                                 */
/*                                                                    */
/* Do a depth first search repeatedly, each time increasing the depth */
/* by one. This allows us to return a reasonable move if we are       */
/* interrupted.                                                       */
/**********************************************************************/

Score 
Search_Engine::iterative_deepening 
(const Board &b, int depth, Move_Vector &pv) 
{
  Score s = search_with_memory (b, 1, pv);

  for (int i = 2; i <= depth; i++) 
    {
      pv.clear ();
      s = search_with_memory (b, i, pv);
    }

  return s;
}

/***********************************************************************/
/* Search_Engine :: search_with_memory ()                              */
/*                                                                     */
/* This routine wraps search and memoizes lookups in the transposition */
/* table.                                                              */
/*                                                                     */
/***********************************************************************/

Score
Search_Engine :: search_with_memory 
(const Board &b, int depth, Move_Vector &pv, Score alpha, Score beta) 
{
  TT_Entry entry;
  bool found_tt_entry;
  Score original_alpha = alpha;

  /*******************************************************************/
  /* Throw away the transposition table when we are approaching a 50 */
  /* move rule draw.                                                 */
  /*******************************************************************/

  if (b.half_move_clock == 40 || 
      b.half_move_clock == 41)
    {
      tt.clear ();
    }

  /*****************************************************/
  /* Try to find this node in the transposition table. */
  /*****************************************************/

  found_tt_entry = tt_fetch (b.hash, entry);
  if (found_tt_entry && entry.depth == depth)
    {
      if (entry.type == TT_Entry::EXACT_VALUE)
	alpha = entry.move.score;
      
      else if (entry.type == TT_Entry::LOWERBOUND)
	alpha = max (entry.move.score, alpha);
      
      else if (entry.type == TT_Entry::UPPERBOUND)
	beta = min (entry.move.score, beta);
    }

  alpha = search (b, depth, pv, alpha, beta);
  
  /****************************************************/
  /* Update the transposition table with this result. */
  /****************************************************/
  if (pv.count > 0 && (!found_tt_entry || depth > entry.depth))
    {
      pv.push (entry.move);

      entry.move = pv[0];
      entry.depth = depth;

      if (alpha <= original_alpha) 
	entry.type = TT_Entry :: LOWERBOUND;

      else if (alpha >= beta)
	entry.type = TT_Entry :: UPPERBOUND;

      else
	entry.type = TT_Entry :: EXACT_VALUE;

      tt_store (b.hash, entry);
    }

  return alpha;
}

Score
Search_Engine :: search 
(const Board &b, int depth, Move_Vector &pv, Score alpha, Score beta) 
{
  pv.clear ();
  bool found_move = false;
  Color player = b.flags.to_move;

  /**********************/
  /* Update statistics. */
  /**********************/

  calls_to_alpha_beta++;

  /**************************************/
  /* Abort if we have been interrupted. */
  /**************************************/

  // if (interrupt_search)
  // throw (SEARCH_INTERRUPTED);

  /******************************/
  /* Handle the leaf node case. */
  /******************************/

  if (b.half_move_clock == 49) 
    {
      alpha = 0;
    }

  /* If repetition, return draw. */
  
  else if (depth == 0) 
    {
      alpha = eval (b);
    }

  /*****************************************************/
  /* Otherwise recurse over the children of this node. */
  /*****************************************************/
  
  else {

#if 0
    // Null move heuristic.
    if (depth % 3 == 0) 
      {
	Move_Vector dummy_pv;
	int rd = depth - 4;
	if (!b.in_check (player) && rd > 0)
	  {
	    Board c = b;
	    c.set_color (invert_color (player));
	    int val = -search_with_memory 
	      (c, rd, dummy_pv, -beta, -beta + 1);
	    if (val >= beta)
	      return beta;
	  }
      }
#endif
	
    Move_Vector moves (b);
    order_moves (b, depth, moves);

    /**************************/
    /* Minimax over children. */
    /**************************/

    for (int i = 0; i < moves.count; i++)
      {
	Board c = b;
	Move_Vector cpv;

	if (c.apply (moves[i]))
	  {
	    found_move = true;
	    int cs = -search_with_memory (c, depth - 1, cpv, -beta, -alpha);

	    if (cs > alpha)
	      {
		alpha = cs;
		moves[i].score = alpha;
		pv = Move_Vector (moves[i], cpv);
	      }

	    if (alpha >= beta)
	      {
		break;
	      }
	  }
      }

    // If we couldn't find at least one legal move the game is over.
    if (!found_move)
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

	    alpha = -MATE_VAL - depth;
	  }
	else
	  {
	    alpha = 0;
	  }
      }
    else
      {
	/**********************************************/
	/* Update the history table with this result. */
	/**********************************************/
	
	hh_table[player][depth][pv[0].from][pv[0].to] += 1 << depth;
      }
  }

  return alpha;
}

// Attempt to order moves to improve our odds of getting earlier
// cutoffs.
void 
Search_Engine::order_moves (const Board &b, int depth, Move_Vector &moves) {
  TT_Entry e;
  bool have_entry = tt_fetch (b.hash, e);

  for (int i = 0; i < moves.count; i++)
    {
      // If we previously computed that moves[i] was the best move
      // from this position, make sure it is searched first.
      if (have_entry && moves[i] == e.move && depth >= e.depth - 1)
	{
	  moves[i].score = +INF;
	}
      else
	{
	  // Award a bonus for rate and depth of recent cutoffs.
	  moves[i].score += hh_table[b.flags.to_move][depth][moves[i].from][moves[i].to];

	  if (depth > 1)
	    moves[i].score += hh_table[b.flags.to_move][depth][moves[i].from][moves[i].to] / 2;

	}
    }
  
  insertion_sort <Move_Vector, Move, less_than> (moves);
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
  else
    {
      out = TT_Entry ();
      return false;
    }
}

// Store an entry in the transposition table.
inline void
Search_Engine::tt_store (uint64 hash, const TT_Entry &in) {
  if ((tt.size () > TT_SIZE)) tt.erase (tt.begin ());
  tt.erase (hash);
  tt.insert (pair <uint64, TT_Entry> (hash, in));
}
