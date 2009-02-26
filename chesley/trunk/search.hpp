/*
  Interface to the search engine. Clients are expected to create and
  configure search engine objects then call its methods to do various
  types of searches.

  Matthew Gingell
  gingell@adacore.com
*/

#ifndef _SEARCH_
#define _SEARCH_

#include <tr1/unordered_map>
#include "board.hpp"

// An entry in the transposition table used to memoize searches. Is
// the best move we have found in a search a particular depth and
// bounds on its value.

struct TT_Entry {
  Move move;
  int depth;
  int upperbound;
  int lowerbound;

  TT_Entry () {
    depth = -1;
    upperbound = INFINITY;
    lowerbound = -INFINITY;
  }

  bool operator == (const TT_Entry &lhs) const {
    return (memcmp (this, &lhs, sizeof (TT_Entry)) == 0);
  }

  bool operator != (const TT_Entry &lhs) const {
    return (memcmp (this, &lhs, sizeof (TT_Entry)) != 0);
  }
};

// A transposition table type mapping from a 64 bit board hash to a
// TT_Entry.

typedef std::tr1::unordered_map <uint64, TT_Entry> Trans_Table;

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

  // Transposition table.
  Trans_Table tt;

  // If set true, the search should conclude as quickly as possible.
  bool interrupt_search;

  /***************/
  /* Statistics. */
  /***************/

  // Count the number of times score has been called.
  uint64 score_count;

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

  Move MTDf (const Board &root, int f, int d);

  // Return the best available move with its score using minimax with
  // alpha-beta pruning.
  Move alpha_beta_with_memory
  (const Board &b, int depth, int alpha = -INFINITY, int beta = INFINITY);

  // Fetch an entry from the transposition table. Returns false if no
  // entry is found.
  bool tt_fetch (uint64 hash, TT_Entry &out);

  // Store an entry from the transposition table.
  void tt_store (uint64 hash, const TT_Entry &in);
};

#endif // _SEARCH_
