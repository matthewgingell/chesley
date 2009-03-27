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

  static const uint32 TT_SIZE = 10 * 1000 * 1000;

  /************************************/
  /* Constructors and initialization. */
  /************************************/

  Search_Engine () {
    interrupt_search = false;
    calls_to_alpha_beta = 0;
    memset (hh_table, 0, sizeof (hh_table));
    tt.rehash (TT_SIZE);
  }

  /*************************/
  /* Transposition tables. */
  /*************************/

  struct TT_Entry {
    Move move;
    int32 depth;
    enum { LOWERBOUND, UPPERBOUND, EXACT_VALUE } type;
  };
  
  // A table type mapping from a 64-bit key to a TT_Entry.
  typedef boost::unordered_map <hash_t, TT_Entry> Trans_Table;

  Trans_Table tt;

  /*********************/
  /* Repetition table. */
  /*********************/
  
  // A table type mapping from a 64-bit key to a repetition count.
  typedef boost::unordered_map <hash_t, int> Rep_Table;

  Rep_Table rt;

  /*****************/
  /* Search state. */
  /*****************/

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

  /*********************************/
  /* Triple repetition management. */
  /*********************************/

  // Add an entry to the repetition table. 
  void rt_push (const Board &b);
  
  // Remove an entry to the repetition table. 
  void rt_pop (const Board &b);
  
  // Test whether this board is a third repetition.
  bool is_triple_rep (const Board &b);

private:

  // History tables. 
  uint64 hh_table[2][100][64][64];

  // Initialize a new search and return its value.
  Score new_search (const Board &b, int depth, Move_Vector &pv);

  // Search repeatedly from depth 1 to 'depth.;
  Score iterative_deepening (const Board &b, int depth, Move_Vector &pv);

  // Memoized minimax search.
  Score search_with_memory 
  (const Board &b, 
   int depth, int ply, 
   Move_Vector &pv, 
   Score alpha = -INF, Score beta = INF,
   bool do_null_move = true);

  // Minimax search.
  Score search 
  (const Board &b, 
   int depth, int ply,
   Move_Vector &pv, 
   Score alpha = -INF, Score beta = INF,
   bool do_null_move = true);

  // Quiescence search. 
  Score qsearch 
  (const Board &b, int depth, int ply,
   Score alpha = -INF, Score beta = INF);

  // Heuristically order a list of moves by value.
  inline void order_moves (const Board &b, int depth, Move_Vector &moves);

  // Fetch a transposition table entry. 
  inline bool tt_fetch (uint64 hash, TT_Entry &out);

  // Store a transposition table entry. 
  inline void tt_store (uint64 hash, const TT_Entry &in);
};

#endif // _SEARCH_
