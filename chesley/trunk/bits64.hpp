/* 
   Operations on a u_int64_t bitmap represention.

   Matthew Gingell     		 
   gingell@adacore.com
*/

#ifndef _BITS64_
#define _BITS64_

#include <iostream>
#include "types.hpp"

typedef uint64 bits64;

#define IS_CONST __attribute__ ((const))
#define IS_UNUSED  __attribute__ ((unused))

/***********************************************/
/* Bitwise operations on 64 bit unsigned ints. */
/***********************************************/

inline bool   test_bit   (bits64, int) IS_CONST;
inline bits64 set_bit    (bits64, int) IS_CONST;
inline bits64 clear_bit  (bits64, int) IS_CONST;
inline bits64 clear_lsb  (bits64)      IS_CONST;
inline bits64 clear_msbs (bits64)      IS_CONST;
inline uint32 bit_idx    (bits64)      IS_CONST;
inline uint32 pop_count  (bits64)      IS_CONST;
inline byte   get_byte   (bits64, int) IS_CONST;
static void   print_bits (bits64)      IS_UNUSED;

// Test a bit.
inline bool 
test_bit (bits64 b, int idx) { return b & 1llu << idx; }

// Set a bit.
inline bits64 
set_bit (bits64 b, int idx) { return b | 1llu << idx; }

// Clear a bit.
inline bits64 
clear_bit (bits64 b, int idx) { return b & ~(1llu << idx); }

// Clear the least significant bit of b.
inline bits64 
clear_lsb (bits64 b) { return b & (b - 1); }

// Clear all but the least significant bit of b.
inline bits64 
clear_msbs (bits64 b) { return b & -b; }

// Return the bit index of the least significant bit. Returns -1 in
// the case b is 0x0.
inline uint32
bit_idx (bits64 b) {
#ifdef __GNUC__
  // Using __builtin_ffsll is a surprisingly big win here.
  return __builtin_ffsll (b) - 1; 
#else
  int c;
  if (!b) return -1;
  b = (b ^ (b - 1llu)) >> 1;
  for (c = 0; b; c++) b >>= 1;
  return c;
#endif
}

// Count the number of bits set in b.
inline u_int32_t 
pop_count (bits64 b) {
  // This is a big win over __builtin_popcount, at 
  // least for our purposes with g++ 4.4.
  uint32 n;
  for (n = 0; b != 0; n++, b = clear_lsb(b));
  return n;
}

// Fetch a byte from a bits64.
inline byte 
get_byte (bits64 b, int n) { 
  return ((byte *) &b)[n]; 
}

/*********/
/* Debug */
/*********/

// Debug routine. 
static void 
print_bits (bits64 b) {
  for (int i = 0; i < 64; i++)
    std::cerr << (test_bit (b, i) ? "1" : "0");
  std::cerr << std:: endl;
}

#endif // _BITS64_
