/*
  Interface to the search engine. Clients are expected to create and
  configure search engine objects then call its methods to do various
  types of searches.

  Matthew Gingell
  gingell@adacore.com
*/

#ifndef _SEARCH_
#define _SEARCH_

#include "board.hpp"

struct Search_Engine {

  /************************************/
  /* Constructors and initialization. */
  /************************************/

  Search_Engine (int max_depth = 4) : max_depth (max_depth) {
    assert (max_depth > 0);
    interrupt_search = false;
    score_count = 0;
  }
  
  /*****************/
  /* Search state. */
  /*****************/

  // Default maximum depth for tree searches.
  int max_depth;

  // If set true, the search should conclude as quickly as possible.
  bool interrupt_search;

  /***************/
  /* Statistics. */
  /***************/

  // Count the number of times score has been called.
  u_int64_t score_count;

  /************/
  /* Queries. */
  /************/

  // Choose a move, score it, and return it.
  Move choose_move (Board &b, int32 depth = -1);

  // Return an estimate of the value of a position.
  int32 score (const Board &b, int32 depth = -1);

private:

  // Do a depth first search repeatedly, each time increasing the
  // depth by one. This allows us to return a reasonable move if we
  // are interrupted.
  Move iterative_deepening (const Board &b, int depth);

  // Return the best available move with its score using minimax with
  // alpha-beta pruning.
  Move alpha_beta
  (const Board &b, int depth, int alpha = -INFINITY, int beta = INFINITY);

};

#endif // _SEARCH_
