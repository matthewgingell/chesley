////////////////////////////////////////////////////////////////////////////////
// phash.cpp                                                                  //
//                                                                            //
// The pawn evaluation cache data type.                                       //        
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009-2010. Chesley        //
// the Chess Engine! is free software distributed under the terms of the      //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _PHASH_
#define _PHASH_

#include <cassert>
#include <stdlib.h>

#include "common.hpp"

struct PHash
{
  // Initialize the table.
  PHash (size_t sz) : 
    sz (sz), hits (0), misses (0), writes (0), collisions (0) {
    table = (Entry *) calloc (sz, sizeof (Entry));
  }

  // An entry in the hash table.
  struct Entry {
    bitboard key;
    Score score;
  };

  // Clear the entire table.
  void clear () {
    memset (table, 0, sz * sizeof (Entry));
    hits = misses = collisions = writes = 0;
  }

  // Set an entry by key.
  void set
  (const bitboard &key, Score s) {
    writes++;
    Entry &e = table[key % sz];
    if (e.key != 0 && e.key != key)
      collisions++;
    e.key = key;
    e.score = s;
  }

  // Find an entry by key.
  bool
  lookup (const bitboard &key, Score &s) {
    Entry &e = table[key % sz];
    if (e.key == key)
      {
        hits++;
        s = e.score;
        return true;
      }
    else
      {
        misses++;
        return false;
      }
  }
  
  // Clear statistics.
  void clear_statistics () {
    hits = misses = writes = collisions = 0;
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

#endif // _PHASH_
