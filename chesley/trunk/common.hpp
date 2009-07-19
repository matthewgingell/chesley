////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// common.hpp                                                                  //
//                                                                            //
// Various simple types.                                                      //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef __COMMON__
#define __COMMON__

#include <cstdio>

#ifndef _WIN32
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
#else
typedef signed char int8;
typedef signed int int32;
typedef unsigned char uint8; 
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned __int64 uint64;
typedef unsigned char byte;
#endif

#ifdef _WIN32
#define snprintf _snprintf 
#endif // _WIN32

// Type for a number representing a square on the board.
typedef uint32 coord;

// Hash type for a chess position.
typedef uint64 hash_t;

//////////////////////////////////
// Symbolic values for squares. //
//////////////////////////////////

enum Square {
  A1, B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8
};

enum File {
  A, B, C, D, E, F, G, H
};

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

inline Color operator! (Color c) { return invert (c); }
inline Color operator~ (Color c) { return invert (c); }

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

////////////////////////////
// Score type and kinds.  //
////////////////////////////

// Score type for a chess position.
typedef int32 Score;

enum SKind {
  NULL_SKIND, LOWER_BOUND, UPPER_BOUND, EXACT_VALUE 
};

////////////////////////////////
// Position evaluation types. //
////////////////////////////////

enum Phase { OPENING, MIDGAME, ENDGAME };

#endif // __COMMON__
