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

struct Game_Over {
  Game_Over (Status status) : status (status) {}
  Status status;
};

std::ostream & operator<< (std::ostream &os, Game_Over e);

struct Search_Engine {

  /************************************/
  /* Constructors and initialization. */
  /************************************/

  Search_Engine (int max_depth = 4) : max_depth (max_depth) {
    assert (max_depth > 0);
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

  // Select a move. Caller must check the value of b.flags.status
  // after this call, since it will be set and an exception raised if
  // no moves can be generated from this position.
  Move choose_move (Board &b);

  // Score a move dynamically.
  int score (const Board &b, int depth, 
	     int alpha = -INFINITY, 
	     int beta = INFINITY);

};

#endif // _SEARCH_
