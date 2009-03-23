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
  // Clear the history table.
  memset (hh_table, 0, sizeof (hh_table));

  // Clear the transposition table.
  tt.clear ();

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
  Score s;
  
  // Search progressively deeper play until we are interrupted.
  for (int i = 1; i <= depth; i++) 
    {
      Move_Vector tmp;
      calls_to_alpha_beta = 0;

      try 
	{
	  s = search_with_memory (b, i, 0, tmp);
	  assert (tmp.count > 0);
	}
      catch (int except)
	{
	  assert (except == SEARCH_INTERRUPTED);
	  cerr << "caught exception at depth = " << i << endl;
	  break;
	}

      // If we got back a principle variation, return it to the caller.
      if (tmp.count > 0) pv = tmp;

      // Show thinking.
      cerr << b.full_move_clock << ":" << b.half_move_clock
	   << ": ply: " << i << " " 
	   << "score: " << pv[0].score << " ";
      for (int j = 0; j < pv.count; j++)
	cerr << b.to_calg (pv[j]) << "(" << pv[j].score << ") ";
      cerr << endl;
    }

  // Check that we got at least one move.
  assert (pv.count > 0);

  return pv[0].score;
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
(const Board &b, int depth, int ply, 
 Move_Vector &pv, Score alpha, Score beta) 
{
  TT_Entry entry;
  bool found_tt_entry;
  Score original_alpha = alpha;

  assert (pv.count == 0);

  /*****************************************************/
  /* Try to find this node in the transposition table. */
  /*****************************************************/

#if 0
  found_tt_entry = tt_fetch (b.hash, entry);
  if (found_tt_entry && entry.depth == depth 
      && b.half_move_clock <= 48) 
    {
      if (entry.type == TT_Entry::EXACT_VALUE)
	alpha = entry.move.score;
      
      else if (entry.type == TT_Entry::LOWERBOUND)
	alpha = max (entry.move.score, alpha);
      
      else if (entry.type == TT_Entry::UPPERBOUND)
	beta = min (entry.move.score, beta);
    }
#endif

  alpha = search (b, depth, ply, pv, alpha, beta);

#if 0
  if (pv.count == 0 && found_tt_entry)
    {
      pv.push (entry.move);
    }
#endif


#if 0
  /****************************************************/
  /* Update the transposition table with this result. */
  /****************************************************/
  if (pv.count > 0 && (!found_tt_entry || depth >= entry.depth))
    {
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
#endif

  return alpha;
  
}

Score
Search_Engine :: search 
(const Board &b, int depth, int ply, 
 Move_Vector &pv, Score alpha, Score beta) 
{
  bool found_move = false;
  Color player = b.flags.to_move;

  assert (pv.count == 0);

  /**************************************/
  /* Abort if we have been interrupted. */
  /**************************************/
  
  if (interrupt_search)
    throw (SEARCH_INTERRUPTED);

  /**********************/
  /* Update statistics. */
  /**********************/

  calls_to_alpha_beta++;

  /********************************************************/
  /* Return the result of a quiescence search at depth 0. */
  /********************************************************/
  
  if (depth == 0) 
    {
      alpha = qsearch (b, -1, ply, alpha, beta);
    }

  /*****************************************************/
  /* Otherwise recurse over the children of this node. */
  /*****************************************************/
  
  else {

#ifdef ENABLE_NULL_MOVE
    /************************/
    /* Null move heuristic. */
    /************************/

    if (depth % 3 == 0) 
      {
	Move_Vector dummy_pv;
	int rd = depth - 3;
	if (!b.in_check (player) && rd > 0)
	  {
	    Board c = b;
	    c.set_color (invert_color (player));
	    int val = -search_with_memory 
	      (c, rd, ply + 1, dummy_pv, -beta, -beta + 1);
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

	    // Recursively score the child.
	    int cs = -search_with_memory 
	      (c, depth - 1, ply + 1, cpv, -beta, -alpha);

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

    // If none of the moves we tried applied, then the game is over.
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

	    alpha = -(MATE_VAL - ply);
	  }
	else
	  {
	    alpha = -(CONTEMPT_VAL - ply);
	  }
      }
    else
      {
	/**********************************************/
	/* Update the history table with this result. */
	/**********************************************/
	if (pv.count > 0)
	  hh_table[player][depth][pv[0].from][pv[0].to] += 1;
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
      if (have_entry && depth <= e.depth && moves[i] == e.move)
	{
	  moves[i].score = +INF;
	}
      else
	{
	  // Award a bonus for rate and depth of recent cutoffs.
	  moves[i].score += 
	    hh_table[b.flags.to_move][depth][moves[i].from][moves[i].to];
	}
    }
  
  insertion_sort <Move_Vector, Move, less_than> (moves);
}

// Quiescence search. 
Score 
Search_Engine::qsearch 
(const Board &b, int depth, int ply, 
 Score alpha, Score beta) 
{
  alpha = max (alpha, eval (b) - ply);

  if (alpha < beta) 
    {
      Board c;
      Move_Vector moves (b);
      bool found_move = false;
 
      // Sort moves on a MVV basis.
      for (int i = 0; i < moves.count; i++) 
	{
	  if (moves[i].capture (b) == NULL_KIND) continue;
	  moves[i].score = moves[i].capture (b);
	}
      insertion_sort <Move_Vector, Move, less_than> (moves);

      // Minimax on captures.
      for (int i = 0; i < moves.count; i++)
	{
	  if (moves[i].capture (b) == NULL_KIND) continue;
	  c = b;
	  if (c.apply (moves[i]))
	    {
	      found_move = true;
	      alpha = max (alpha, -qsearch (c, depth - 1, ply + 1, -beta, -alpha));
	      if (alpha >= beta) break;
	    }
	}

      // We can not ignore non-capturing moves in the case this may be a
      // draw or checkmate.
      if (!found_move) 
	{
	  // Exit if we find at least one legal move.
	  for (int i = 0; i < moves.count; i++)
	    {
	      if (moves[i].capture (b) != NULL_KIND) continue;
	      c = b;
	      if (c.apply (moves[i])) 
		{
		  found_move = true;
		  break;
		}
	    }

	  // Otherwise, this is check or checkmate. 
	  if (!found_move)
	    {
	      if (b.in_check (b.flags.to_move))
		{
		  alpha = -(MATE_VAL - ply);
		}
	      else 
		{
		  alpha = -(CONTEMPT_VAL - ply);
		}
	    }
	}
    }

  return alpha;
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
  if ((tt.size () > TT_SIZE)) 
    tt.clear ();
    
  tt.erase (hash);
  tt.insert (pair <uint64, TT_Entry> (hash, in));
}
