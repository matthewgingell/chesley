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
#include "board.hpp"
#include "eval.hpp"
#include "ttable.hpp"
#include "util.hpp"
#include <boost/unordered_map.hpp>

// Supported time keeping modes.
enum time_mode { CONVENTIONAL, ICS, EXACT };

// Maximum search depth. 
static const int32 MAX_DEPTH = 256;
static const int32 MAX_PLY = 256;
static const int hist_nbuckets = 10;

struct Search_Engine {

  ////////////////
  // Constants. //
  ////////////////

  // Transposition table size in entries. This should be a power of 2.
  static const uint32 TT_SIZE = 4 * 1024 * 1024;

  //////////////////////////////////////
  // Constructors and initialization. //
  //////////////////////////////////////

  Search_Engine () : tt (TT_SIZE) {
    reset ();
  }

  // Reset all search engine state to defaults.
  void reset () {

    // Clear search statistics.
    clear_statistics ();

    // Time management parameters.
    controls.mode = EXACT;
    controls.moves_ptc = -1;
    controls.time_ptc = -1;
    controls.increment = -1;
    controls.fixed_time = -1;
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

    // Initialize the history table.
    ZERO (hh_table);
    hh_max = 0;

    // Initialize the mates table.
    ZERO (mates_table)
    mates_max = 0;

  }

  // Clear accumulated search statistics.
  void clear_statistics () {
    tt.clear_statistics ();
    stats.asp_hits = 0;
    stats.calls_to_qsearch = 0;
    stats.calls_to_search = 0;
    stats.delta_count = 0;
    stats.ext_count = 0;
    stats.ext_futility_count = 0;
    stats.futility_count = 0;
    stats.lmr_count = 0;
    stats.null_count = 0;
    stats.razor_count = 0;
    ZERO (stats.calls_for_depth);
    ZERO (stats.time_for_depth);
    ZERO (stats.hist_pv);
    ZERO (stats.hist_qpv);
  }

  //////////////////////////////////////////
  // Transposition and repetition tables. //
  //////////////////////////////////////////

  // Transposition table instance.
  TTable tt;

  // Try to get a move or tighten the window from the transposition
  // table, returning true if we found a move we can return at this
  // position.
  inline bool tt_try
  (const Board &b, int32 depth, int32 ply,
   Move &m, Score &s, Score &alpha, Score &beta);

  // Update the transposition table with the results of a call to
  // search.
  inline void tt_update
  (const Board &b, int32 depth, int32 ply,
   const Move_Vector &pv, Score s, int32 alpha, int32 beta);

  // A table type mapping from a 64-bit key to a repetition count.
  typedef boost::unordered_map <hash_t, int> Rep_Table;

  // Fetch a move from the transposition table.
  inline Move tt_move (const Board &b);

#if 0
  // Extend the principal variation from the transposition table.
  inline void tt_extend_pv (const Board &b, Move_Vector &pv);
#endif

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

  // Poll is called periodically either by the caller or during search.
  inline void poll ();

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
  void post_each (const Board &b, int depth, Score s, const Move_Vector &pv);

  // Write thinking output after iterative deeping ends.
  void post_after ();

  /////////////////
  // Statistics. //
  /////////////////

  // The start time of some operation being timed.
  uint64 start_time;

  // Count the number of times search and qsearch have been called.
  struct {
    uint64 calls_to_qsearch;
    uint64 calls_to_search;
    uint64 calls_for_depth[MAX_DEPTH];
    uint64 time_for_depth[MAX_DEPTH];

    // A histogram of times we found a PV node at an index into the
    // moves list. This is a measure of the performance of our move
    // ordering strategy.
    
    uint64 hist_pv [hist_nbuckets];
    uint64 hist_qpv [hist_nbuckets];
    
    // Counts of hits for various heuristics.
    uint64 asp_hits;
    uint64 delta_count;
    uint64 ext_count;
    uint64 ext_futility_count;
    uint64 futility_count;
    uint64 lmr_count;
    uint64 null_count;
    uint64 razor_count;
  } stats;

  ///////////////////////////////////
  // Hierarchy of search routines. //
  //////////////////////////////////

  // Choose a move, score it, and return it.
  Move choose_move (const Board &b, int32 depth = -1);

  // Initialize a new search and return its value.
  Score new_search (const Board &b, int depth, Move_Vector &pv);
  
  // Setup a deadline for this search.
  void new_deadline ();

  // Search repeatedly from depth 1 to 'depth.;
  Score iterative_deepening (const Board &b, int depth, Move_Vector &pv);

  // Search a root node.
  Score root_search (const Board &b, int depth, Move_Vector &pv, Score guess = 0);

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
  Score see (const Board &b, const Move &capture) const;

  // Heuristically order a list of moves by estimated value.
  void order_moves (const Board &b, int ply, Move_Vector &moves);
  
  // Return on a depth adjustment for a position.
  int depth_adjustment (const Board &b, Move m);

  /////////////////
  // Heuristics. //
  /////////////////

  uint64 hh_table[64][64];
  uint64 hh_max;

  uint64 mates_table[64][64];
  uint64 mates_max;

  void collect_move (int depth, int ply, const Move &m, Score s);
};

#endif // _SEARCH_
