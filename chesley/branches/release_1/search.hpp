////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// search.hpp                                                                 //
//                                                                            //
// Interface to the search engine. Clients are expected to create and         //
// configure search engine objects then call its methods to do various        //
// types of searches.                                                         //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _SEARCH_
#define _SEARCH_

#include <cstring>
#include <boost/unordered_map.hpp>
#include "board.hpp"
#include "eval.hpp"

// Supported time keeping modes.
enum time_mode { CONVENTIONAL, ICS, EXACT };

struct Search_Engine {

  ////////////////
  // Constants. //
  ////////////////

  // Transposition table size in entries.
  static const uint32 TT_SIZE = 2.5 * 1000 * 1000;

  // Default fixed time per move in milliseconds.
  static const uint32 DEFAULT_TIMEOUT = 1 * 1000;

  // Maximum search depth. 
  static const int32 MAX_DEPTH = 100;

  //////////////////////////////////////
  // Constructors and initialization. //
  //////////////////////////////////////

  Search_Engine () {
    reset ();
  }

  // Reset all search engine state to defaults.
  void reset () {

    // Time management parameters.
    controls.mode = EXACT;
    controls.moves_ptc = -1;
    controls.time_ptc = -1;
    controls.increment = -1;
    controls.fixed_time = DEFAULT_TIMEOUT;
    controls.fixed_depth = -1;
    controls.time_remaining = -1;
    controls.moves_remaining = -1;
    controls.interrupt_search = false;
    controls.deadline = 0;

    // Transposition and repetition tables.
    tt.clear ();
    rt.clear ();

    // Output generation.
    post = true;

    // Search statistics.
    clear_statistics ();

    // Initialize history and killer tables. 
    memset (hh_table, 0, sizeof (hh_table));
    memset (killers, 0, sizeof (killers));
  }

  // Clear accumulated search statistics.
  void clear_statistics () {
    calls_to_search = 0;
    calls_to_qsearch = 0;
    tt_hits = 0;
    tt_misses = 0;
    memset (hist_pv, 0, sizeof (hist_pv));
  }

  //////////////////////////////////////////
  // Transposition and repetition tables. //
  //////////////////////////////////////////

  // Transposition table entry.
  enum Node_Type { LOWERBOUND, UPPERBOUND, EXACT_VALUE };
  struct TT_Entry {
    Move move;
    Node_Type type;
    int32 depth;
    int32 age;
  };

  // Transposition table type.
  typedef boost::unordered_map <hash_t, TT_Entry> Trans_Table;

  // Transposition table instance.
  Trans_Table tt;

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

  // A table type mapping from a 64-bit key to a repetition count.
  typedef boost::unordered_map <hash_t, int> Rep_Table;

  // The repetition table.
  Rep_Table rt;

  // Add an entry to the repetition table.
  void rt_push (const Board &b);

  // Remove an entry from the repetition table.
  void rt_pop (const Board &b);

  // Fetch the repetition count for a position.
  int rep_count (const Board &b);

  // Test whether this board is a repetition.
  bool is_rep (const Board &b);

  // Test whether this board is a third repetition.
  bool is_triple_rep (const Board &b);

  ///////////////////////////////////////////////////////////////////////
  // Time management. Times are always expected to be in milliseconds. //
  ///////////////////////////////////////////////////////////////////////

  // The caller is responsible for periodically calling the poll
  // method while a search is underway. It's important that the caller
  // manage this and not the search object, since there may be many
  // searches but only a few timers.
  void poll (uint64 clock);

  // Set fixed time per move in milliseconds.
  void set_fixed_depth (int depth);

  // Set fixed depth per move.
  void set_fixed_time (int time);

  // Setup time controls corresponding to and xboard "level" command.
  void set_level (int mptc, int tptc, int inc);

  // Set the time remaining before the game clock expires.
  void set_time_remaining (int msecs);

  struct {

    //////////////////////////////////////////////////////////////
    // Time controls as configured but the user. In all cases a //
    // negative number indicates that no limit was been set.    //
    //////////////////////////////////////////////////////////////

    time_mode mode;
    int moves_ptc;       // Moves per time control.
    int time_ptc;        // Milliseconds per time control.
    int increment;       // Increment in ICS mode.
    int fixed_time;      // Fixed milliseconds per move.
    int fixed_depth;     // Absolute fixed depth per move.

    ////////////////////////////////////////////////////////////////
    // Resources remaining for this games. In all case a negative //
    // number indicates a resource is unlimited.                  //
    ////////////////////////////////////////////////////////////////

    int time_remaining;  // Time remaining for this game.
    int moves_remaining; // Moves remaining for this game.

    /////////////////////////////////////////////////////////////
    // Resources remaining for the search currently underway.  //
    /////////////////////////////////////////////////////////////

    uint64 deadline;       // Deadline after which to halt.
    bool interrupt_search; // If true, return as soon as possible.

  } controls;

  ////////////////////////
  // Output generation. //
  ////////////////////////

  // If true, thinking output will be written to stdout.
  bool post;

  // Write thinking output before iterative deeping starts.
  void post_before (const Board &b);

  // Write thinking for each depth during iterative deeping.
  void post_each (const Board &b, int depth, const Move_Vector &pv);

  // Write thinking output after iterative deeping ends.
  void post_after ();

  /////////////////
  // Statistics. //
  /////////////////

  // The start time of some operation being timed.
  uint64 start_time;

  // Count the number of times search and qsearch have been called.
  uint64 calls_to_search;
  uint64 calls_to_qsearch;

  // A histogram of times we found a PV node at an index into the
  // moves list. This is a measure of the performance of our move
  // ordering strategy.
  static const int hist_nbuckets = 10;
  uint32 hist_pv [hist_nbuckets];

  // Count the number of times we hit and miss entries in the
  // transposition table.
  uint64 tt_hits;
  uint64 tt_misses;

  ///////////////////////////////////
  // Hierarchy of search routines. //
  //////////////////////////////////

  // Choose a move, score it, and return it.
  Move choose_move (Board &b, int32 depth = -1);

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
  inline void order_moves (const Board &b, int depth, Move_Vector &moves);

  /////////////////
  // Heuristics. //
  /////////////////

  // History heuristic table.
  uint64 hh_table[2][MAX_DEPTH][64][64];

  // Killer moves.
  Move killers[MAX_DEPTH][2];

};

#endif // _SEARCH_