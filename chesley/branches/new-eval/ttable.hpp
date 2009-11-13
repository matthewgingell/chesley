////////////////////////////////////////////////////////////////////////////////
// ttable.cpp                                                                 //
//                                                                            //
// The transposition table data type.                                         //        
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _TTABLE_
#define _TTABLE_

#include <cassert>
#include <stdlib.h>

#include "common.hpp"
#include "move.hpp"

struct TTable
{
  // Initialize the table.
  TTable (size_t sz) : 
    sz (sz), hits (0), misses (0), writes (0), collisions (0) {
    table = (Entry *) calloc (sz, sizeof (Entry));
  }

  // An entry in the hash table.
  struct Entry {
    hash_t key;    // :64
    Move   move;   // :32
    Score  score;  // :16
    int    depth   : 8;
    SKind  skind   : 8;
  };

  // Clear the entire table.
  void clear () {
    memset (table, 0, sz * sizeof (Entry));
    hits = misses = collisions = writes = 0;
  }

  // Set an entry by key.
  void set
  (const Board &b, SKind k, Move m, Score s, int d) {
    Entry &e = table[b.hash % sz];
    
    // Collect statistics.
    writes++;
    if (e.key != 0 && e.key != b.hash)
      collisions++;

    e.key = b.hash;
    e.move = m;
    e.score = s;
    e.depth = d;
    e.skind = k;
  }

  // Find an entry by key.
  SKind 
  lookup (const Board &b, Move &m, Score &s, int &d) {
    Entry &e = table[b.hash % sz];
    if (e.key == b.hash && e.skind != NULL_SKIND) 
      {
        hits++;
        m = e.move;
        s = e.score;
        d = e.depth;
        return e.skind;
      }
    else
      {
        misses++;
        m = NULL_MOVE;
        s = 0;
        d = 0;
        return NULL_SKIND;
      }
  }

  // Fetch the move, if any, associated with this position.
  Move get_move (const Board &b) {
    Entry &e = table[b.hash % sz];
    if (e.key == b.hash && e.skind != NULL_SKIND)
      {
        hits++;
        return e.move;
      }
    else
      {
        misses++;
      }
    return NULL_MOVE;
  }
  
  // Clear statistics.
  void clear_statistics () {
    hits = misses = writes = collisions = 0;
  }

  // Return true is nothing is stored in a table slot.
  bool free_entry (const Board &b) {
    return (table[b.hash % sz].key == 0);
  }

  // Data.
  size_t sz;
  Entry *table;

  // Statistics.
  uint64 hits;
  uint64 misses;
  uint64 writes;
  uint64 collisions;
};

#endif // _TTABLE_
