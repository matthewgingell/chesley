/*
  Interface to the search engine. Clients are expected to create and
  configure search engine objects then call its methods to do various
  types of searches.

  Matthew Gingell
  gingell@adacore.com
*/

#ifndef _SEARCH_
#define _SEARCH_

#include <boost/unordered_map.hpp>

#include "board.hpp"
#include "eval.hpp"

struct Search_Engine {

  /************************************/
  /* Constructors and initialization. */
  /************************************/

  Search_Engine (int max_depth = 4) : max_depth (max_depth) {
    assert (max_depth > 0);
    interrupt_search = false;
    calls_to_alpha_beta = 0;
  }

  /*************************/
  /* Transposition tables. */
  /*************************/

  struct TT_Entry {
    TT_Entry () { depth = -1;}
    Move move;
    int32 depth;
    enum { LOWERBOUND, UPPERBOUND, EXACT_VALUE } type;
  };
  
  // A table type mapping from a 64-bit key to a TT_Entry.
  typedef boost::unordered_map <uint64, TT_Entry> Trans_Table;

  Trans_Table tt;

  /******************/
  /* History tables */
  /******************/


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

  // Count the number of times alpha beta has been called.
  uint64 calls_to_alpha_beta;

  /************/
  /* Queries. */
  /************/

  // Choose a move, score it, and return it.
  Move choose_move (Board &b, int32 depth = -1);

  // Return an estimate of the value of a position.
  score_t score (const Board &b, int32 depth = -1);

  // Fetch the principle variation for the most recent search.
  void fetch_pv (const Board &b, Move_Vector &out);

private:

  // Do a depth first search repeatedly, each time increasing the
  // depth by one. This allows us to return a reasonable move if we
  // are interrupted.
  Move iterative_deepening (const Board &b, int depth);

  Move MTDf (const Board &root, int f, int d);

  // Return the best available move with its score using minimax with
  // alpha-beta pruning.
  Move alpha_beta
  (const Board &b, int depth, 
   score_t alpha = -INF, score_t beta = +INF);

  // Attempt to order moves to improve our odds of getting earlier
  // cutoffs.
  void order_moves (const Board &b, Move_Vector &moves);

  // Fetch an entry from the transposition table. Returns false if no
  // entry is found.
  bool tt_fetch (uint64 hash, TT_Entry &out);

  // Store an entry from the transposition table.
  void tt_store (uint64 hash, const TT_Entry &in);
};

#endif // _SEARCH_
