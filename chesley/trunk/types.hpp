////////////////////////////////////////////////////////////////////////////////
// 								     	      //
// types.hpp							     	      //
// 								     	      //
// Various simple types.          					      //
// 								     	      //
// Matthew Gingell							      //
// gingell@adacore.com							      //
// 								     	      //
////////////////////////////////////////////////////////////////////////////////

#ifndef __TYPES__
#define __TYPES__

#include <stdint.h>

typedef int8_t   int8;
typedef int16_t  int16;
typedef int32_t  int32;
typedef int64_t  int64;
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef uint8_t  byte;

// Score type for a chess position.
typedef int32 Score;

// Hash type for a chess position.
typedef uint64 hash_t;

////////////////////
// Piece colors.  //
////////////////////

// This type is used to index tables and the ordering here should not
// be changed.
enum Color { 
  NULL_COLOR = -1, WHITE = 0, BLACK = 1 
};

inline Color invert (Color c) { return (c == WHITE) ? BLACK : WHITE; }
inline int sign (Color c) { return (c == WHITE) ? +1 : -1; }

inline void operator++ (Color &c, int) { c = (Color) (c + 1); }
inline void operator-- (Color &c, int) { c = (Color) (c - 1); }
inline void operator++ (Color &c) { c = (Color) (c + 1); }
inline void operator-- (Color &c) { c = (Color) (c - 1); }

std::ostream & operator<< (std::ostream &os, Color c);

//////////////////
// Piece kinds. //
////////////////// 

// This type is used to index tables and the ordering here should not
// be changed.
enum Kind { 
  NULL_KIND = -1, PAWN = 0, ROOK, KNIGHT, BISHOP, QUEEN, KING 
};

inline void operator++ (Kind &k, int) { k = (Kind) (k + 1); }
inline void operator-- (Kind &k, int) { k = (Kind) (k - 1); }
inline void operator++ (Kind &k) { k = (Kind) (k + 1); }
inline void operator-- (Kind &k) { k = (Kind) (k - 1); }

char to_char (Kind k);
Kind to_kind (char k);

std::ostream & operator<< (std::ostream &os, Kind k);

#endif // __TYPES__
