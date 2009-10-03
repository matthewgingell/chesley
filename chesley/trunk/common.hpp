////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// common.hpp                                                                 //
//                                                                            //
// Various declarations common to all units.                                  //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef __COMMON__
#define __COMMON__

#include <algorithm>
#include <cassert>

///////////
// Types //
///////////

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
typedef signed short int16;
typedef signed int int32;
typedef signed __int64 int64;
typedef unsigned char uint8; 
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned __int64 uint64;
typedef unsigned char byte;
#endif

#ifdef _WIN32
#include <windows.h>
#include <winbase.h>
#ifndef snprintf
#define snprintf _snprintf
#endif
#ifndef fileno
#define fileno _fileno
#endif
#endif // _WIN32

// Type for a number representing a square on the board.
typedef uint32 Coord;

// Hash type for a chess position.
typedef uint64 hash_t;

// Bitboard type
typedef uint64 bitboard;

/////////////////////////////////
// Symbolic values for squares //
/////////////////////////////////

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

const int RANK_COUNT = 8;
const int FILE_COUNT = 8;

// This type is used to index tables and the ordering here should not
// be changed.
enum Color { NULL_COLOR = -1, WHITE = 0, BLACK = 1 };

const int COLOR_COUNT = 2;

// This type is used to index tables and the ordering here should not
// be changed.

enum Kind {
  NULL_KIND = -1, PAWN = 0, ROOK, KNIGHT, BISHOP, QUEEN, KING
};

const int KIND_COUNT = 6;

// Score type for a chess position.
typedef int16 Score;

enum SKind {
  NULL_SKIND, LOWER_BOUND, UPPER_BOUND, EXACT_VALUE 
};

// Game phase type.
enum Phase { OPENING, MIDGAME, ENDGAME };

// Castling rights.
enum Castling_Right {
  W_QUEEN_SIDE, W_KING_SIDE, B_QUEEN_SIDE, B_KING_SIDE
};

////////////////////
// Initialization //
////////////////////

// Must be called to initialize tables in the correct order.
void precompute_tables ();

///////////////
// Utilities //
///////////////

inline Color invert (Color c) { return (c == WHITE) ? BLACK : WHITE; }
inline int sign (Color c) { return (c == WHITE) ? +1 : -1; }

inline void operator++ (Color &c, int) { c = (Color) (c + 1); }
inline void operator-- (Color &c, int) { c = (Color) (c - 1); }
inline void operator++ (Color &c) { c = (Color) (c + 1); }
inline void operator-- (Color &c) { c = (Color) (c - 1); }

inline Color operator! (Color c) { return invert (c); }
inline Color operator~ (Color c) { return invert (c); }

std::ostream & operator<< (std::ostream &os, Color c);

inline void operator++ (Kind &k, int) { k = (Kind) (k + 1); }
inline void operator-- (Kind &k, int) { k = (Kind) (k - 1); }
inline void operator++ (Kind &k) { k = (Kind) (k + 1); }
inline void operator-- (Kind &k) { k = (Kind) (k - 1); }

char to_char (Kind k);
Kind to_kind (char k);

std::ostream & operator<< (std::ostream &os, Kind k);

inline bool in_bounds (int x, int y);
inline bitboard rank_mask (int rank);
inline bitboard in_front_of_mask (Coord idx, Color c);
inline bitboard file_mask (int file);
inline bitboard this_file_mask (Coord idx);
inline int idx_to_rank (Coord idx);
inline int idx_to_file (Coord idx);
inline Coord to_idx (int rank, int file);
inline hash_t get_zobrist_piece_key (Color c, Kind k, Coord idx);

// Test whether a coordinate is in bounds.
inline bool
in_bounds (int x, int y) {
  return x >= 0 && x <= 7 && y >= 0 && y <= 7;
}

// Return a bitboard with every bit of the Nth rank set.
inline bitboard
rank_mask (int rank) {
  return 0x00000000000000FFULL << rank * 8;
}

// Return a bitboard with every bit of the Nth file set.
inline bitboard
file_mask (int file) {
  return 0x0101010101010101ULL << file;
}

// Return a bitboard with every bit of the Nth file set.
inline bitboard
this_file_mask (Coord idx) {
  return 0x0101010101010101ULL << idx_to_file (idx);
}

// Return the rank 0 .. 7 containing a coordinate.
inline int
idx_to_rank (Coord idx) {
  return idx / 8;
}

// Return the file 0 .. 7 containing a coordinate.
inline int
idx_to_file (Coord idx) {
  return idx % 8;
}
  
// Return an index for a rank and file.
inline Coord 
to_idx (int rank, int file) {
  return 8 * rank + file;
}

// Return the distance between two locations.
inline uint32
dist (Coord a, Coord b) {
  return std::max 
    (std::abs (idx_to_file (a) - idx_to_file (b)),
     std::abs (idx_to_rank (a) - idx_to_rank (b)));
}

extern uint64 *zobrist_piece_keys;
extern uint64 *zobrist_enpassant_keys;
extern uint64  zobrist_key_white_to_move;
extern uint64  zobrist_w_castle_q_key;
extern uint64  zobrist_w_castle_k_key;
extern uint64  zobrist_b_castle_q_key;
extern uint64  zobrist_b_castle_k_key;

// Fetch the key for a piece.
inline hash_t 
get_zobrist_piece_key (Color c, Kind k, Coord idx) {
  uint32 i = (c == BLACK ? 0 : 1);
  uint32 j = (int32) k;
  assert (c != NULL_COLOR);
  assert (k != NULL_KIND);
  assert (idx < 64);
  assert ((i * (64 * 6) + j * (64) + idx) < 2 * 6 * 64);
  return zobrist_piece_keys[i * (64 * 6) + j * (64) + idx];
}

//////////////////////////////////////
// Precomputed tables and constants //
//////////////////////////////////////

extern const std::string INITIAL_POSITIONS;

static const bitboard light_squares = 0x55AA55AA55AA55AAULL;
static const bitboard dark_squares = 0xAA55AA55AA55AA55ULL;

static const int flip_left_right[64] =
  {
     7,  6,  5,  4,  3,  2,  1,  0,
    15, 14, 13, 12, 11, 10,  9,  8,
    23, 22, 21, 20, 19, 18, 17, 16,
    31, 30, 29, 28, 27, 26, 25, 24,
    39, 38, 37, 36, 35, 34, 33, 32,
    47, 46, 45, 44, 43, 42, 41, 40,
    55, 54, 53, 52, 51, 50, 49, 48,
    63, 62, 61, 60, 59, 58, 57, 56
  };

static const int flip_white_black[64] =
  {
    56,  57,  58,  59,  60,  61,  62,  63,
    48,  49,  50,  51,  52,  53,  54,  55,
    40,  41,  42,  43,  44,  45,  46,  47,
    32,  33,  34,  35,  36,  37,  38,  39,
    24,  25,  26,  27,  28,  29,  30,  31,
    16,  17,  18,  19,  20,  21,  22,  23,
     8,   9,  10,  11,  12,  13,  14,  15,
     0,   1,   2,   3,   4,   5,   6,   7
  };

extern bool have_precomputed_tables;

extern bitboard *KNIGHT_ATTACKS_TBL;
extern bitboard *KING_ATTACKS_TBL;
extern bitboard *RANK_ATTACKS_TBL;
extern bitboard *FILE_ATTACKS_TBL;
extern bitboard *DIAG_45_ATTACKS_TBL;
extern bitboard *DIAG_135_ATTACKS_TBL;

extern byte *KNIGHT_MOBILITY_TBL;
extern byte *KING_MOBILITY_TBL;
extern byte *RANK_MOBILITY_TBL;
extern byte *FILE_MOBILITY_TBL;
extern byte *DIAG_45_MOBILITY_TBL;
extern byte *DIAG_135_MOBILITY_TBL;

extern bitboard *masks_0;
extern bitboard *masks_45;
extern bitboard *masks_90;
extern bitboard *masks_135;

extern int *rot_45;
extern int *rot_90;
extern int *rot_135;

extern byte *diag_shifts_45;
extern byte *diag_bitpos_45;
extern byte *diag_widths_45;
extern byte *diag_shifts_135;
extern byte *diag_bitpos_135;
extern byte *diag_widths_135;

// Bitboard masks for detection various features.
extern bitboard *pawn_attack_spans[2];
extern bitboard *in_front_of[2];
extern bitboard *adjacent_files;

////////////////////////////////////
// Patterns for use in evaluation //
////////////////////////////////////
  
// Return a bitboard of all squares in front on this square.
inline bitboard in_front_of_mask (Coord idx, Color c) { 
  return in_front_of[c][idx]; 
}

// Return the files adjacent to this one.
inline bitboard
adjacent_files_mask (Coord idx) {
  return adjacent_files[idx];
}

// Return a mask of squares adjacent to idx.
inline bitboard 
adjacent_square (Coord idx) {
  return KING_ATTACKS_TBL[idx];
}


#endif // __COMMON__
