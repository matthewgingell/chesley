/*
  Interface to the search engine. Clients are expected to create and
  configure search engine objects then call its methods to do various
  types of searches.

  Matthew Gingell
  gingell@adacore.com
*/

#ifndef _SEARCH_
#define _SEARCH_

#include <cstring>
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
    memset (hh_table, 0, sizeof (hh_table));
  }

  /*************************/
  /* Transposition tables. */
  /*************************/

  struct TT_Entry {
    TT_Entry () : move(0), depth (0) {}
    Move move;
    int32 depth;
    enum { LOWERBOUND, UPPERBOUND, EXACT_VALUE } type;
  };
  
  // A table type mapping from a 64-bit key to a TT_Entry.
  typedef boost::unordered_map <hash_t, TT_Entry> Trans_Table;

  Trans_Table tt;

  /******************/
  /* History tables */
  /******************/

  int hh_table[64][64];

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

  // Fetch the principle variation for the most recent search.
  void fetch_pv (const Board &b, Move_Vector &out);

private:

  // Initialize a new search and return its value.
  Score new_search (const Board &b, int depth);

  // Memoized minimax search.
  Score search_with_memory (const Board &b, int depth, Move_Vector &pv, 
			    Score alpha = -INF, Score beta = INF);

  // Minimax search.
  Score search (const Board &b, int depth, Move_Vector &pv, 
		Score alpha = -INF, Score beta = INF);

  // Fetch a transposition table entry. 
  inline bool tt_fetch (uint64 hash, TT_Entry &out);

  // Store a transposition table entry. 
  inline void tt_store (uint64 hash, const TT_Entry &in);

  // Heuristically order a list of moves by value.
  inline void order_moves (const Board &b, Move_Vector &moves);

  // Search repeatedly from depth 1 to 'depth.;
  Score iterative_deepening (const Board &b, int depth);
};

#endif // _SEARCH_
