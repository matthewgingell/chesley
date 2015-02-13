////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// bits64.hpp                                                                 //
//                                                                            //
// Operations on 64-bit bitmaps.                                              //
//                                                                            //
// Copyright Matthew Gingell <matthewgingell@gmail.com>, 2009-2015.           //
// Chesley the Chess Engine! is free software distributed under the terms of  //
// the GNU Public License.                                                    //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _BITS64_
#define _BITS64_

#include <iostream>

#include "common.hpp"
#include "util.hpp"

typedef uint64 bits64;

////////////////////////////////////////////////
// Bitwise operations on 64 bit unsigned ints //
////////////////////////////////////////////////

inline bool   test_bit   (bits64,   int)   IS_CONST;
inline void   set_bit    (bits64 &, int);
inline void   clear_bit  (bits64 &, int);
inline bits64 clear_lsb  (bits64)          IS_CONST;
inline bits64 clear_msbs (bits64)          IS_CONST;
inline uint32 bit_idx    (bits64)          IS_CONST;
inline uint32 pop_count  (bits64)          IS_CONST;
inline byte   get_byte   (bits64,   int)   IS_CONST;

////////////////////////
// Debugging routines //
////////////////////////

static void print_bits  (bits64 b) IS_UNUSED;
static void print_board (bits64 b) IS_UNUSED;

// Test a bit.
inline bool
test_bit (bits64 b, int idx) {
  return (b & 1ULL << idx);
}

// Set a bit.
inline void
set_bit (bits64 &b, int idx) {
  b |= 1ULL << idx;
}

// Clear a bit.
inline void
clear_bit (bits64 &b, int idx) {
  b &= ~(1ULL << idx);
}

// Clear the least significant bit of b.
inline bits64
clear_lsb (bits64 b) {
  return b & (b - 1);
}

// Clear all but the least significant bit of b.
inline bits64
clear_msbs (bits64 b) {
  return b & -((int64) b);
}

// Return the bit index of the least significant bit. Returns -1 in
// the case b is 0x0.
inline uint32
bit_idx (bits64 b) {
#if defined (__GNUC__) || defined (__INTEL_COMPILER)
    return __builtin_ffsll (b) - 1;
#else
  int c;
  if (!b) return -1;
  b = (b ^ (b - 1ULL)) >> 1;
  for (c = 0; b; c++) b >>= 1;
  return c;
#endif
}

// Count the number of bits set in b.
inline uint32
pop_count (bits64 b) {
#ifdef __GNUC__
  return __builtin_popcountll (b);
#else
  uint32 n;
  for (n = 0; b != 0; n++, b = clear_lsb(b)) {};
  return n;
#endif
}

// Fetch a byte from a bits64.
inline byte
get_byte (bits64 b, int n) {
  return ((byte *) &b)[n];
}

////////////////////////
// Debugging Routines //
////////////////////////

// Print a bit vector.
static void
print_bits (bits64 b) {
  for (int i = 0; i < 64; i++)
    std::cerr << (test_bit (b, i) ? "1" : "0");
  std::cerr << std:: endl;
}

// Print a 64 bit set as a 8x8 matrix of 'X; and '.'.
static void
print_board (bits64 b) {
  for (int y = 7; y >= 0; y--)
    {
      for (int x = 0; x < 8; x++)
        std::cerr << (test_bit (b, x + 8 * y) ? 'X' : '.') << " ";
      std::cerr << std::endl;
    }
  std::cerr << std::endl;
}


#endif // _BITS64_
