/*
  genmask.cpp

  Code to generate and initialize masks used in addressing bitboards
  and computing piece moves. Each set of masks is an 64 entry array
  of single bit masks where the Nth entry corresponds to the Nth bit.

  Each 45 degree increment rotates the board one half step
  counterclockwise. There is a detailed and very readable description
  of this approch in:

  _Rotated bitboards in FUSc#_ by Johannes Buchner:
  http://page.mi.fu-berlin.de/~fusch/publications/Joe-Paper_rotated_bitboards.pdf

  Matthew Gingell
  gingell@adacore.com
*/

#include "chesley.hpp"

/*
  Masks for unrotated bitboards. This is trivial but provided for the
  sake of consistency.

  56 57 58 59 60 61 62 63      56 57 58 59 60 61 62 63
  48 49 50 51 52 53 54 55      48 49 50 51 52 53 54 55
  40 41 42 43 44 45 46 47      40 41 42 43 44 45 46 47
  32 33 34 35 36 37 38 39  ==> 32 33 34 35 36 37 38 39
  24 25 26 27 28 29 30 31      24 25 26 27 28 29 30 31
  16 17 18 19 20 21 22 23      16 17 18 19 20 21 22 23
  08 09 10 11 12 13 14 15      08 09 10 11 12 13 14 15
  00 01 02 03 04 05 06 07      00 01 02 03 04 05 06 07
*/

bitboard *
Board::init_masks_0 () {
  bitboard *masks = new bits64[64];
  for (int i = 0; i < 64; i++)
      masks[i] = 1llu << i;
  return masks;
}

/*
  Masks for bitboards rotated 45 degrees:

  Rot 0:

  56 57 58 59 60 61 62 63
  48 49 50 51 52 53 54 55
  40 41 42 43 44 45 46 47
  32 33 34 35 36 37 38 39  ==>
  24 25 26 27 28 29 30 31
  16 17 18 19 20 21 22 23
  08 09 10 11 12 13 14 15
  00 01 02 03 04 05 06 07


               63
             62  55
           61  54  47                 Rot 45:
         60  53  46  39
       59  52  45  38  31              46 39 61 54 55 62 55 63
     58  51  44  37  30  23            23 59 52 45 38 31 60 53
   57  50  43  36  29  22  15          29 22 15 58 51 44 37 30
 56  49  42  35  28  21  14  07   ==>  28 21 14 07 57 50 43 36
   48  41  34  27  20  13  06          27 20 13 06 56 49 42 35
     40  33  26  19  12  05            33 26 18 11 04 48 41 34
       32  25  18  11  04              10 03 32 25 18 11 04 40
         24  17  10  03                00 08 01 16 09 02 24 17
           16  09  02
             08  01
               00

*/


// Rotate normal indices 45 degrees.
int *
Board::init_rot_45 () {
  int *rot = new int[64];
  int idx_00 = 0;

  // Lower diagonal.
  for (int i = 0; i <= 56; i += 8) 
    for (int j = i; j >= (i / 8); j -= 7)
      rot[idx_00++] = j;

  // Upper diagonal.
  for (int i = 57; i < 64; i++)
    for (int j = i; j >= 15 + 8 * (i - 57); j -= 7)
      rot[idx_00++] = j;
      
  return rot;
}

// Table of normal index to shift to fetch occupancy pattern.
byte *
Board :: init_diag_shifts_45 () {
  static byte rotated[64];
  static byte unrotated[] =
    {
      0,
      1,  1,
      3,  3,  3,
      6,  6,  6,  6,
      10, 10, 10, 10, 10,
      15, 15, 15, 15, 15, 15,
      21, 21, 21, 21, 21, 21, 21,
      28, 28, 28, 28, 28, 28, 28, 28,
      36, 36, 36, 36, 36, 36, 36,
      43, 43, 43, 43, 43, 43,
      49, 49, 49, 49, 49,
      54, 54, 54, 54,
      58, 58, 58,
      61, 61,
      63,
    };

  assert (rot_45 != 0);

  for (int i = 0; i < 64; i++)
    rotated[rot_45[i]] = unrotated[i];

  return rotated;
}

// Find the position of a bit within a diagonal occupancy byte.
byte *
Board :: init_diag_bitpos_45  () {
  static byte rotated[64];
  static byte unrotated[] =
    {
      0,
      0, 1,
      0, 1, 2,
      0, 1, 2, 3,
      0, 1, 2, 3, 4,
      0, 1, 2, 3, 4, 5,
      0, 1, 2, 3, 4, 5, 6,
      0, 1, 2, 3, 4, 5, 6, 7,
      0, 1, 2, 3, 4, 5, 6,
      0, 1, 2, 3, 4, 5,
      0, 1, 2, 3, 4,
      0, 1, 2, 3,
      0, 1, 2,
      0, 1,
      0,
    };

  assert (rot_45 != 0);

  for (int i = 0; i < 64; i++)
    rotated [rot_45[i]] = unrotated[i];

  return rotated;
}

// Find the width of a diagonal given a normal position.
byte *
Board :: init_diag_widths_45 () {
  static byte rotated[64];
  static byte unrotated[] =
  {
    1,
    2, 2,
    3, 3, 3,
    4, 4, 4, 4,
    5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8,
    7, 7, 7, 7, 7, 7, 7,
    6, 6, 6, 6, 6, 6,
    5, 5, 5, 5, 5,
    4, 4, 4, 4,
    3, 3, 3,
    2, 2,
    1
  };
  
  assert (rot_45 != 0);

  for (int i = 0; i < 64; i++)
    rotated[rot_45[i]] = unrotated[i];

  return rotated;
}

bitboard *
Board::init_masks_45 () {
  bits64 *masks = new bits64[64];
  int idx_00 = 0;

  // Lower diagonal.
  for (int i = 0; i <= 56; i += 8)
      for (int j = i; j >= (i / 8); j -= 7)
	masks[j] = 1llu << idx_00++;

  // Upper diagonal.
  for (int i = 57; i < 64; i++)
    for (int j = i; j >= 15 + 8 * (i - 57); j -= 7)
      masks[j] = 1llu << idx_00++;

  return masks;
}

/*
  Masks for bitboards rotated 90 degrees:

  Rot 0:                      Rot 90:

  56 57 58 59 60 61 62 63     63 55 47 39 31 23 15 07
  48 49 50 51 52 53 54 55     62 54 46 38 30 22 14 06
  40 41 42 43 44 45 46 47     61 53 45 37 29 21 13 05
  32 33 34 35 36 37 38 39 ==> 60 52 44 36 28 20 12 04
  24 25 26 27 28 29 30 31     59 51 43 35 27 19 11 03
  16 17 18 19 20 21 22 23     58 50 42 34 26 18 10 02
  08 09 10 11 12 13 14 15     57 49 41 33 25 17 09 01
  00 01 02 03 04 05 06 07     56 48 40 32 24 16 08 00
*/

bitboard *
Board::init_masks_90 () {
  bitboard *masks = new bits64[64];

  int idx_00 = 0;
  for (int i = 56; i < 64; i++)
    for (int j = i;  j >= i % 56; j -= 8)
      masks[j] = 1llu << idx_00++;

  return masks;
}

int *
Board::init_rot_90 () {
  int *rot = new int[64];

  int idx_00 = 0;
  for (int i = 56; i < 64; i++)
    for (int j = i;  j >= i % 56; j -= 8)
      rot[idx_00++] = j;

  return rot;
}

/*
  Rot 0:

  56 57 58 59 60 61 62 63
  48 49 50 51 52 53 54 55
  40 41 42 43 44 45 46 47
  32 33 34 35 36 37 38 39  ==>
  24 25 25 27 28 29 30 31
  16 17 18 19 20 21 22 23
  08 09 10 11 12 13 14 15
  00 01 02 03 04 05 06 07

   Rot 135:

               07
             15  06
           23  14  05
         31  22  13  04
       39  30  21  12  03             13 04 23 14 05 15 06 07
     47  38  29  20  11  02           02 39 30 21 12 03 31 22
   55  46  37  28  19  10  01         19 10 01 47 38 29 20 11
 63  54  45  36  27  18  09  00  ==>  27 18 09 00 55 46 37 28
   62  53  44  35  26  17  08         35 26 17 08 63 54 45 36
     61  52  43  34  25  16           52 43 34 25 16 62 53 44
       60  51  42  33  24             41 32 60 51 42 33 24 61
         59  50  41  32               56 57 48 58 50 42 59 50
           58  49  40
             57  48
               56
*/

// Table of normal index => bit shift to fetch occupancy pattern..
byte *
Board :: init_diag_shifts_135 () {
  static byte rotated[64];
  static byte unrotated[] =
    {
      0,
      1,  1,
      3,  3,  3,
      6,  6,  6,  6,
      10, 10, 10, 10, 10,
      15, 15, 15, 15, 15, 15,
      21, 21, 21, 21, 21, 21, 21,
      28, 28, 28, 28, 28, 28, 28, 28,
      36, 36, 36, 36, 36, 36, 36,
      43, 43, 43, 43, 43, 43,
      49, 49, 49, 49, 49,
      54, 54, 54, 54,
      58, 58, 58,
      61, 61,
      63,
    };

  assert (rot_135 != 0);

  for (int i = 0; i < 64; i++)
    rotated[rot_135[i]] = unrotated[i];

  return rotated;
}

// Find the position of a bit within a diagonal occupancy byte.
byte *
Board :: init_diag_bitpos_135  () {
  static byte rotated[64];
  static byte unrotated[] =
    {
      0,
      0, 1,
      0, 1, 2,
      0, 1, 2, 3,
      0, 1, 2, 3, 4,
      0, 1, 2, 3, 4, 5,
      0, 1, 2, 3, 4, 5, 6,
      0, 1, 2, 3, 4, 5, 6, 7,
      0, 1, 2, 3, 4, 5, 6,
      0, 1, 2, 3, 4, 5,
      0, 1, 2, 3, 4,
      0, 1, 2, 3,
      0, 1, 2,
      0, 1,
      0,
    };

  assert (rot_135 != 0);

  for (int i = 0; i < 64; i++)
    rotated [rot_135[i]] = unrotated[i];

  return rotated;
}

// Find the width of a diagonal given a normal position.
byte *
Board :: init_diag_widths_135  () {
  static byte rotated[64];
  static byte unrotated[] =
  {
    1,
    2, 2,
    3, 3, 3,
    4, 4, 4, 4,
    5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8,
    7, 7, 7, 7, 7, 7, 7,
    6, 6, 6, 6, 6, 6,
    5, 5, 5, 5, 5,
    4, 4, 4, 4,
    3, 3, 3,
    2, 2,
    1
  };
  
  assert (rot_135 != 0);

  for (int i = 0; i < 64; i++)
    rotated[rot_135[i]] = unrotated[i];

  return rotated;
}

bitboard *
Board::init_masks_135 () {
  bitboard *masks = new bits64[64];
  int idx_00 = 0;

  // Lower diagonal.
  for (int i = 56; i <= 63; i++)
    for (int j = i; j >= 56 - (i % 56) * 8; j -= 9)
      masks[j] = 1llu << idx_00++;

  // Upper diagonal.
  for (int i = 55; i > 0; i -= 8)
    for (int j = i; j >= i % 9; j -= 9)
      masks[j] = 1llu << idx_00++;

  return masks;
}

int *
Board::init_rot_135 () {
  int *rot = new int[64];
  int idx_00 = 0;

  // Lower diagonal.
  for (int i = 56; i <= 63; i++)
    for (int j = i; j >= 56 - (i % 56) * 8; j -= 9)
      rot[idx_00++] = j;

  // Upper diagonal.
  for (int i = 55; i > 0; i -= 8)
    for (int j = i; j >= i % 9; j -= 9)
      rot[idx_00++] = j;

  return rot;
}

// Debug
void print_masks (bitboard *b) {
  for (int i = 0; i < 64; i++)
    print_bits (b[i]);
}
