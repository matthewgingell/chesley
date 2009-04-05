#include <cassert>
#include <stdlib.h>
#include <unistd.h>
#include "types.hpp"

struct Trans_Table 
{
  struct Entry { 
    hash_t key; 
    int32_t depth, from, to, upperbound, lowerbound; 
  };

  Trans_Table (size_t sz = 10 * 1024 * 1024) : sz (sz) {
    lookups = collisions = 0;
    table = (Entry *) calloc (sz, sizeof (Entry));
    assert (table);
  }

  void clear () {
    free (table);
    table = (Entry *) calloc (sz, sizeof (Entry));
    assert (table);
  }

  size_t find_slot (hash_t key) {
    size_t i = key % sz;
    lookups++;
    while (i < sz) 
      {
	if (table [i].key == 0 || table [i].key == key) return i;
	collisions++;
	i++;
      }

    return 0;
  }

  void store (hash_t key, Entry &e) {
    table[find_slot (key)] = e;
  }

  bool fetch (hash_t key, Entry &e) {
    e = table[find_slot (key)];
    return (e.key != 0);
  }

  int lookups;
  int collisions;
  size_t sz;
  Entry *table;
};
