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
#include <unistd.h>
#include "move.hpp"
#include "types.hpp"

struct Trans_Table
{
  struct Entry {
    hash_t key;
    Move   move;
    Score  score;
    int    depth;
    Node_Type type;
  };

  Trans_Table (size_t sz = 1 << 20) : sz (sz) {
    table = (Entry *) calloc (sz, sizeof (Entry));
  }

  void clear () {
    memset (table, 0, sz * sizeof (Entry));
  }

  void store (hash_t key, Entry &e) {
    table[key % sz] = e;
  }

  bool fetch (hash_t key, Entry &e) {
    e = table[key % sz];
    return (key == e.key);
  }

  size_t sz;
  Entry *table;
};

#endif // _TTABLE_
