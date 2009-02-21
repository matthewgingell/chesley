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
#include "eval.hpp"

/*******************************************/
/* Exception to raise if the game is over. */
/*******************************************/

struct Search_Engine {

  /************************************/
  /* Constructors and initialization. */
  /************************************/

  Search_Engine (int max_depth = 4) : max_depth (max_depth) {
    assert (max_depth > 0);
    interrupt_search = false;
    score_count = 0;
  }
  
  /****************************/
  /* Configuration variables. */
  /****************************/

  int max_depth;

  /***************/
  /* Statistics. */
  /***************/

  // Count the number of times score has been called.
  u_int64_t score_count;

  /************/
  /* Queries. */
  /************/

  // If set true, the searh should conclude as quickly as possible.
  bool interrupt_search;

  // Select a move. Caller must check the value of b.flags.status
  // after this call, since it will be set and an exception raised if
  // no moves can be generated from this position.
  Move choose_move (Board &b);

  // Score a move dynamically.
  int32 score 
  (const Board &b, int depth, int alpha = -INFINITY, int beta = INFINITY);

};

#endif // _SEARCH_
