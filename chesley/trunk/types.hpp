/*
  Various simple types.

  Matthew Gingell
  gingell@adacore.com
*/

#ifndef __TYPES__
#define __TYPES__

// Standard integer types.

#include <stdint.h>

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef uint8 byte;

// Score type for a chess position.
typedef int32 Score;

// Hash type for a chess position.
typedef uint64 hash_t;

// This type is used to index tables, so PAWN must always be set to
// zero, etc.
enum Kind { NULL_KIND = -1, PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING };

inline void operator++ (Kind &k) {
  k = (Kind) (k + 1);
}

inline void operator-- (Kind &k) {
  k = (Kind) (k - 1);
}

// Convert a kind to a character code.
char to_char (Kind k);

// Convert a character code to a piece code, ignoring color.
Kind to_kind (char k);
std::ostream & operator<< (std::ostream &os, Kind k);

#endif // __TYPES__
