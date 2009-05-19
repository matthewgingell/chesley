////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// search.hpp                                                                 //
//                                                                            //
// Interface to the search engine. Clients are expected to create and         //
// configure search engine objects then call its methods to do various        //
// types of searches.                                                         //
//                                                                            //
// Matthew Gingell                                                            //
// gingell@adacore.com                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _SEARCH_
#define _SEARCH_

#include <cstring>
#include <boost/unordered_map.hpp>
#include "board.hpp"
#include "eval.hpp"

struct Search_Engine {

  static const uint32 TT_SIZE = 2.5 * 1000 * 1000;

  //////////////////////////////////////
  // Constructors and initialization. //
  //////////////////////////////////////

  Search_Engine () {
    interrupt_search = false;
    calls_to_search = 0;
    calls_to_qsearch = 0;
    post = true;
    tt_hits = 0;
    tt_misses = 0;
    memset (hh_table, 0, sizeof (hh_table));
    memset (hist_pv, 0, sizeof (hist_pv));
  }

  ///////////////////////////
  // Transposition tables. //
  ///////////////////////////

  struct TT_Entry {
    Move move;
    int32 depth;
    enum { 
      LOWERBOUND, UPPERBOUND, EXACT_VALUE 
    } type;
    int32 age;
  };
  
  // A table type mapping from a 64-bit key to a TT_Entry.
  typedef boost::unordered_map <hash_t, TT_Entry> Trans_Table;

  Trans_Table tt;

  ///////////////////////
  // Repetition table. //
  ///////////////////////
  
  // A table type mapping from a 64-bit key to a repetition count.
  typedef boost::unordered_map <hash_t, int> Rep_Table;
  Rep_Table rt;

  ///////////////////
  // Search state. //
  ///////////////////

  // If set true, the search should conclude as quickly as possible.
  bool interrupt_search;

  // If true, thinking output will be written.
  bool post;

  /////////////////
  // Statistics. //
  /////////////////

  // The start time of some operation being timed.
  uint64 start_time;

  // Count the number of times search and qsearch have been called.
  uint64 calls_to_search; 
  uint64 calls_to_qsearch; 

  // A count of the number of times we found a PV node at an index
  // into the moves list. This is a measure of the performance of our
  // move ordering strategy.
  static const int hist_nbuckets = 10;
  uint32 hist_pv [hist_nbuckets];
  
  // Count the number of times we hit and miss entries in the
  // transposition table.
  uint64 tt_hits; 
  uint64 tt_misses; 

  //////////////
  // Queries. //
  //////////////

  // Choose a move, score it, and return it.
  Move choose_move (Board &b, int32 depth = -1);

  ///////////////////////////////////
  // Triple repetition management. //
  ///////////////////////////////////

  // Add an entry to the repetition table. 
  void rt_push (const Board &b);
  
  // Remove an entry to the repetition table. 
  void rt_pop (const Board &b);

  // Fetch the repetition count for a position.
  int rep_count (const Board &b);
  
  // Test whether this board is a third repetition.
  bool is_rep (const Board &b);
  
  // Test whether this board is a repetition.
  bool is_triple_rep (const Board &b);

  ///////////////////////////////////
  // Hierarchy of search routines. //
  //////////////////////////////////

  // Initialize a new search and return its value.
  Score new_search (const Board &b, int depth, Move_Vector &pv);

  // Search repeatedly from depth 1 to 'depth.;
  Score iterative_deepening (const Board &b, int depth, Move_Vector &pv);

  // Try a search with a with a plus or minus hw window around
  // 'best_guess' and only do a full width search if that fails.
  Score aspiration_search 
  (const Board &b, int depth, Move_Vector &pv, Score best_guess, Score hw);
  
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
  (const Board &b, int depth, int ply, Score alpha = -INF, Score beta = INF);

  // Static exchange evaluation.
  inline Score see (const Board &b, const Move &capture);

  // Heuristically order a list of moves by value.
  inline void order_moves 
  (const Board &b, int depth, Move_Vector &moves, int alpha, int beta);

  /////////////////////////////////////
  // Transposition table management. //
  /////////////////////////////////////

  // Try to get a move or tighten the window from the transposition
  // table, returning true if we found a move we can return at this
  // position.
  inline bool tt_try 
  (const Board &b, int32 depth, Move &m, int32 &alpha, int32 &beta);

  // Update the transposition table with the results of a call to
  // search.
  inline void tt_update
  (const Board &b, int32 depth, const Move &m, int32 alpha, int32 beta);
  
  // Fetch a transposition table entry. 
  inline bool tt_fetch (uint64 hash, TT_Entry &out);

  //////////////////////////////////////
  // Thinking and statistical output. //
  //////////////////////////////////////

  // Write thinking output before iterative deeping starts.
  void post_before (const Board &b);

  // Write thinking for each depth during iterative deeping.
  void post_each (const Board &b, int depth, const Move_Vector &pv);

  // Write thinking output after iterative deeping ends.
  void post_after ();
  
  ///////////////////////////////////////
  // History heuristic implementation. //
  ///////////////////////////////////////

  uint64 hh_table[2][100][64][64];
};

#endif // _SEARCH_
