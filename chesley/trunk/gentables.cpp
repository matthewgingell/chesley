////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// gentables.cpp                                                              //
//                                                                            //
// Code to generate and initialize masks used in addressing bitboards         //
// and computing piece moves. Each set of masks is an 64 entry array of       //
// single bit masks where the Nth entry corresponds to the Nth bit.           //
//                                                                            //
// Each 45 degree increment rotates the board one half step                   //
// counterclockwise. There is a detailed and very readable description        //
// of this approach in:                                                       //
//                                                                            //
// _Rotated bitboards in FUSc#_ by Johannes Buchner:                          //
// page.mi.fu-berlin.de/~fusch/publications/Joe-Paper_rotated_bitboards.pdf   //
//                                                                            //
// Additional, attack tables for each type of piece are precomputed here.     //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include "chesley.hpp"

/////////////////////////////////////////
// Construction of precomputed tables. //
/////////////////////////////////////////

bool have_precomputed_tables = false;

// Move generation tables.
bitboard *KNIGHT_ATTACKS_TBL;
bitboard *KING_ATTACKS_TBL;
bitboard *RANK_ATTACKS_TBL;
bitboard *FILE_ATTACKS_TBL;
bitboard *DIAG_45_ATTACKS_TBL;
bitboard *DIAG_135_ATTACKS_TBL;

// Mobility tables.
byte *KNIGHT_MOBILITY_TBL;
byte *KING_MOBILITY_TBL;
byte *RANK_MOBILITY_TBL;
byte *FILE_MOBILITY_TBL;
byte *DIAG_45_MOBILITY_TBL;
byte *DIAG_135_MOBILITY_TBL;

// Rotated bitboard tables.
bitboard *masks_0;
bitboard *masks_45;
bitboard *masks_90;
bitboard *masks_135;

int *rot_45;
int *rot_90;
int *rot_135;

byte *diag_shifts_45;
byte *diag_bitpos_45;
byte *diag_widths_45;

byte *diag_shifts_135;
byte *diag_bitpos_135;
byte *diag_widths_135;

// Zobrist hashing tables.
uint64 *zobrist_piece_keys;
uint64 *zobrist_enpassant_keys;
uint64  zobrist_key_white_to_move;
uint64  zobrist_w_castle_q_key;
uint64  zobrist_w_castle_k_key;
uint64  zobrist_b_castle_q_key;
uint64  zobrist_b_castle_k_key;

// Tables used during evaluation.
bitboard *pawn_attack_spans[2];
bitboard *in_front_of[2];
bitboard *adjacent_files;

/////////////////////
// Initialization. //
/////////////////////

// Bit masks for rotated coordinates.
bitboard *init_masks_0 ();
bitboard *init_masks_45 ();
bitboard *init_masks_90 ();
bitboard *init_masks_135 ();

// Maps from normal to rotated coordinates.
int *init_rot_45 ();
int *init_rot_90 ();
int *init_rot_135 ();

// Assessors for diagonals.
byte *init_diag_shifts_45 ();
byte *init_diag_bitpos_45 ();
byte *init_diag_widths_45 ();
byte *init_diag_shifts_135 ();
byte *init_diag_bitpos_135 ();
byte *init_diag_widths_135 ();

// Attack tables.
bitboard *init_knight_attacks_tbl ();
bitboard *init_king_attacks_tbl ();
bitboard *init_rank_attacks_tbl ();
bitboard *init_file_attacks_tbl ();
bitboard *init_45d_attacks_tbl ();
bitboard *init_135d_attacks_tbl ();

// Mobility tables.
static void init_mobility_tables ();

// Zobrist keys.
static void init_zobrist_keys ();

// Evaluation tables.
static void init_in_front_of ();
static void init_pawn_attack_spans ();
static void init_adjacent_files ();

// Precompute all tables.
void
precompute_tables () {
  masks_0 = init_masks_0 ();
  masks_45 = init_masks_45 ();
  masks_90 = init_masks_90 ();
  masks_135 = init_masks_135 ();

  rot_45 = init_rot_45 ();
  rot_90 = init_rot_90 ();
  rot_135 = init_rot_135 ();

  diag_shifts_45 = init_diag_shifts_45 ();
  diag_bitpos_45 = init_diag_bitpos_45 ();
  diag_widths_45 = init_diag_widths_45 ();

  diag_shifts_135 = init_diag_shifts_135 ();
  diag_bitpos_135 = init_diag_bitpos_135 ();
  diag_widths_135 = init_diag_widths_135 ();

  KNIGHT_ATTACKS_TBL = init_knight_attacks_tbl ();
  KING_ATTACKS_TBL = init_king_attacks_tbl ();
  RANK_ATTACKS_TBL = init_rank_attacks_tbl ();
  FILE_ATTACKS_TBL = init_file_attacks_tbl ();
  DIAG_45_ATTACKS_TBL = init_45d_attacks_tbl ();
  DIAG_135_ATTACKS_TBL = init_135d_attacks_tbl ();

  init_mobility_tables ();
  init_zobrist_keys ();
  init_pawn_attack_spans ();
  init_in_front_of ();
  init_adjacent_files ();

  have_precomputed_tables = true;
}

///////////////////////////////////////////////////////////////////
// Generate tables and masks for maintaining rotated bit boards. //
///////////////////////////////////////////////////////////////////

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
init_masks_0 () {
  bitboard *masks = new bits64[64];
  for (int i = 0; i < 64; i++)
      masks[i] = 1ULL << i;
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
init_rot_45 () {
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
init_diag_shifts_45 () {
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
init_diag_bitpos_45  () {
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
init_diag_widths_45 () {
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
init_masks_45 () {
  bits64 *masks = new bits64[64];
  int idx_00 = 0;

  // Lower diagonal.
  for (int i = 0; i <= 56; i += 8)
      for (int j = i; j >= (i / 8); j -= 7)
        masks[j] = 1ULL << idx_00++;

  // Upper diagonal.
  for (int i = 57; i < 64; i++)
    for (int j = i; j >= 15 + 8 * (i - 57); j -= 7)
      masks[j] = 1ULL << idx_00++;

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
init_masks_90 () {
  bitboard *masks = new bits64[64];

  int idx_00 = 0;
  for (int i = 56; i < 64; i++)
    for (int j = i;  j >= i % 56; j -= 8)
      masks[j] = 1ULL << idx_00++;

  return masks;
}

int *
init_rot_90 () {
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
init_diag_shifts_135 () {
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
init_diag_bitpos_135  () {
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
init_diag_widths_135  () {
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
init_masks_135 () {
  bitboard *masks = new bits64[64];
  int idx_00 = 0;

  // Lower diagonal.
  for (int i = 56; i <= 63; i++)
    for (int j = i; j >= 56 - (i % 56) * 8; j -= 9)
      masks[j] = 1ULL << idx_00++;

  // Upper diagonal.
  for (int i = 55; i > 0; i -= 8)
    for (int j = i; j >= i % 9; j -= 9)
      masks[j] = 1ULL << idx_00++;

  return masks;
}

int *
init_rot_135 () {
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

////////////////////////////////////////////////
// Generate tables for generating piece moves //
////////////////////////////////////////////////

// Precompute bitboards for rank attack.
bitboard *
init_rank_attacks_tbl () {
  bitboard *rv = new bitboard[64 * 256];

  // For each position on the board.
  for (int from = 0; from < 64; from++)
    {
      bitboard destinations;
      int first_bit = from - from % 8;
      int last_bit = first_bit + 7;

      // For each possible rank state vector
      for (uint32 occ = 0; occ < 256; occ++)
        {
          destinations = 0ULL;

          // Compute moves to the left of from.
          int k = 0;
          do
            {
              k--;
              if (from + k < first_bit) break;
              set_bit (destinations, from + k);
            }
          while (!test_bit (occ, from % 8 + k));

          // Compute moves to the right of from.
          k = 0;
          do
            {
              k++;
              if (from + k > last_bit) break;
              set_bit (destinations, from + k);
            }
          while (!test_bit (occ, from % 8 + k));

          rv[from * 256 + occ] = destinations;
        }
    }

  return rv;
}

// Precompute bitboards for file attack.
bitboard *
init_file_attacks_tbl () {
  bitboard *rv = new bitboard[64 * 256];

  // For each position on the board.
  for (int from = 0; from < 64; from++)
    {
      bitboard destinations;
      uint32 from_bit = 7 - from / 8;

      // For each possible file state vector
      for (uint32 file_pat = 0; file_pat < 256; file_pat++)
        {
          int bitnum;
          int bit_offset;

          destinations = 0ULL;

          // Compute moves up the board.
          bitnum = from_bit;
          do
            {
              bitnum--;
              if (bitnum < 0) break;
              bit_offset = from_bit - bitnum;
              set_bit (destinations, from + 8 * bit_offset);
            }
          while (!test_bit (file_pat, bitnum));

          // Compute moves down the board.
          bitnum = from_bit;
          do
            {
              bitnum++;
              if (bitnum > 7) break;
              bit_offset = from_bit - bitnum;
              set_bit (destinations, from + 8 * bit_offset);
            }
          while (!test_bit (file_pat, bitnum));

          // Collect moves in destinations bitboard.
          rv[from * 256 + file_pat] = destinations;
        }
    }

  return rv;
}

// Precompute bitboards for knight moves.
bitboard *
init_knight_attacks_tbl()
{
  const int moves[8][2] =
    {{ 2, 1}, { 1, 2}, {-1, 2}, {-2, 1},
     {-2,-1}, {-1,-2}, { 1,-2}, { 2,-1}};

  bitboard *rv = new bitboard[64];

  for (int off = 0; off < 64; off++)
    {
      int x = off % 8;
      int y = off / 8;

      rv[off] = 0;
      for (int v = 0; v < 8; v++)
        {
          int nx = x + moves[v][0];
          int ny = y + moves[v][1];

          if (in_bounds (nx, ny))
            {
              set_bit (rv[off], nx + ny * 8);
            }
        }
    }
  return rv;
}

// Precompute bitboards for king moves.
bitboard *
init_king_attacks_tbl()
{
  const int moves[8][2] =
    {{ 1, 0}, { 1, 1}, { 0, 1}, {-1, 1},
     {-1, 0}, {-1,-1}, { 0,-1}, { 1,-1}};

  bitboard *rv = new bitboard[64];

  for (int off = 0; off < 64; off++)
    {
      int x = off % 8;
      int y = off / 8;

      rv[off] = 0;
      for (int v = 0; v < 8; v++)
        {
          int nx = x + moves[v][0];
          int ny = y + moves[v][1];

          if (in_bounds (nx, ny))
            {
              set_bit (rv[off], nx + ny * 8);
            }
        }
    }
  return rv;
}

// Precompute attacks along the 56 - 07 diagonal.
bitboard *
init_45d_attacks_tbl () {
  bitboard *rv = new bitboard[64 * 256];

  // For each position on the board.
  for (int from = 0; from < 64; from++)
    {
      bitboard destinations;
      int from_bit = diag_bitpos_45[from];
      int pat_len = diag_widths_45[from];
      int b;

      // For each pattern
      for (int pat = 0; pat < 256; pat++)
        {
          destinations = 0ULL;

          // Down and to the right.
          b = from_bit;
          do
            {
              b++;
              if (b >= pat_len) break;
              set_bit (destinations, from + (from_bit - b) * 7);
            }
          while (!test_bit(pat, b));

          // Up and to the left.
          b = from_bit;
          do
            {
              b--;
              if (b < 0) break;
              set_bit (destinations, from + (from_bit - b) * 7);
            }
          while (!test_bit(pat, b));

          rv[from * 256 + pat] = destinations;
        }
    }

  return rv;
}

// Precompute attacks along the 0 - 63 diagonal.
bitboard *
init_135d_attacks_tbl () {
  bitboard *rv = new bitboard[64 * 256];

  // For each position on the board.
  for (int from = 0; from < 64; from++)
    {
      bitboard destinations;
      int from_bit = diag_bitpos_135[from];
      int pat_len = diag_widths_135[from];
      int b;

      // For each pattern
      for (int pat = 0; pat < 256; pat++)
        {
          destinations = 0ULL;

          // Down and to the left.
          b = from_bit;
          do
            {
              b++;
              if (b >= pat_len) break;
              set_bit (destinations, from + (from_bit - b) * 9);
            }
          while (!test_bit(pat, b));

          // Up and to the left.
          b = from_bit;
          do
            {
              b--;
              if (b < 0) break;
              set_bit (destinations, from + (from_bit - b) * 9);
            }
          while (!test_bit(pat, b));

          rv[from * 256 + pat] = destinations;
        }
    }

  return rv;
}

///////////////////////////////////////////////////////////////////////
// Generate tables of random bit-vectors for use as Zobrist hashing. //
///////////////////////////////////////////////////////////////////////

// Initialize all the above keys to random bit strings.
void
init_zobrist_keys () {
  zobrist_key_white_to_move = random64();

  zobrist_piece_keys = new uint64[2 * 6 * 64];
  for (int i = 0; i < 2 * 6 * 64; i++)
    zobrist_piece_keys[i] = random64();

  zobrist_enpassant_keys = new uint64[64];
  zobrist_enpassant_keys[0] = 0;
  for (int i = 1; i < 64; i++)
    zobrist_enpassant_keys[i] = random64();

  zobrist_w_castle_q_key = random64 ();
  zobrist_w_castle_k_key = random64 ();
  zobrist_b_castle_q_key = random64 ();
  zobrist_b_castle_k_key = random64 ();
}

///////////////////////////////
// Generate mobility tables. //
///////////////////////////////

static void init_mobility_tables () {
  // Allocate tables.
  KNIGHT_MOBILITY_TBL =   new byte[64];
  KING_MOBILITY_TBL =     new byte[64];
  RANK_MOBILITY_TBL =     new byte[64 * 256];
  FILE_MOBILITY_TBL =     new byte[64 * 256];
  DIAG_45_MOBILITY_TBL =  new byte[64 * 256]; 
  DIAG_135_MOBILITY_TBL = new byte[64 * 256];
  
  // Compute a population count for each moves bitboard.
  for (int i = 0; i < 64; i++)
    {
      KNIGHT_MOBILITY_TBL[i] = 
        pop_count (KNIGHT_ATTACKS_TBL[i]);
      KING_MOBILITY_TBL[i] = 
        pop_count (KING_ATTACKS_TBL[i]);
    }

  for (int i = 0; i < 64 * 256; i++)
    {
      RANK_MOBILITY_TBL[i] = 
        pop_count (RANK_ATTACKS_TBL[i]);
      FILE_MOBILITY_TBL[i] = 
        pop_count (FILE_ATTACKS_TBL[i]);
      DIAG_45_MOBILITY_TBL[i] = 
        pop_count (DIAG_45_ATTACKS_TBL[i]);
      DIAG_135_MOBILITY_TBL[i] = 
        pop_count (DIAG_135_ATTACKS_TBL[i]);
    }


}

//////////////////////////////////////////////////////
// Generate tables used during position evaluation. //
//////////////////////////////////////////////////////

// Build a table indexed by [color][square] of the two squares
// diagonally ahead of a pawn and all those squares in front of
// them. This is used for detecting pawn sentries.
static void 
init_pawn_attack_spans () {
  // Allocate table.
  pawn_attack_spans[WHITE] = new bitboard[64];
  pawn_attack_spans[BLACK] = new bitboard[64];

  for (coord idx = 0; idx < 64; idx++)
    {
      int rank = idx_to_rank (idx);
      int file = idx_to_file (idx);

      // Entry for white.
      pawn_attack_spans[WHITE][idx] = 0;
      for (int r = rank + 1; r < 8; r++)
        {
          if (file > 0) 
            pawn_attack_spans[WHITE][idx] |= 
              masks_0[to_idx(r, file - 1)];
          if (file < 7) 
            pawn_attack_spans[WHITE][idx] |= 
              masks_0[to_idx(r, file + 1)];
        }
      
      // Entry for black.
      pawn_attack_spans[BLACK][idx] = 0;
      for (int r = rank - 1; r >= 0; r--)
        {
          if (file > 0) 
            pawn_attack_spans[BLACK][idx] |= 
              masks_0[to_idx (r, file - 1)];
          if (file < 7) 
            pawn_attack_spans[BLACK][idx] |= 
              masks_0[to_idx(r, file + 1)];
        }
    }
}

// Build a table indexed by [color][square] of the every square in
// front of a piece.
static void 
init_in_front_of () {
  // Allocate table.
  in_front_of[WHITE] = new bitboard[64];
  in_front_of[BLACK] = new bitboard[64];
  for (coord idx = 0; idx < 64; idx++)
    {
      int rank = idx_to_rank (idx);

      // Entry for white.
      in_front_of[WHITE][idx] = 0;
      for (int r = rank + 1; r < 8; r++)
        for (int f = 0; f < 8; f++)
        {
          in_front_of [WHITE][idx] |= 
            masks_0[to_idx (r, f)];
        }
      
      // Entry for black.
      in_front_of[BLACK][idx] = 0;
      for (int r = rank - 1; r >= 0; r--)
        for (int f = 0; f < 8; f++)
        {
          in_front_of [BLACK][idx] |= 
            masks_0[to_idx (r, f)];
        }
    }
}

// Build a table indexed by [square] of every square on an adjacent
// file.
static void 
init_adjacent_files () {
  // Allocate table.
  adjacent_files = new bitboard[64];

  for (coord idx = 0; idx < 64; idx++)
    {
      int f = idx_to_file (idx);
      adjacent_files[idx] = 0;
      for (int r = 0; r < 8; r++)
        {
          if (f < 7) 
            adjacent_files[idx] |= 
              masks_0[to_idx (r, f + 1)];
          if (f > 0) 
            adjacent_files[idx] |= 
              masks_0[to_idx (r, f - 1)];
        }
    }
}
