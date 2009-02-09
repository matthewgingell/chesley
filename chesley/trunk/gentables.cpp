/* 
   gentables.cpp

   Code to generate precomputed tables of bit vectors representing the
   positions attacked by a piece of some kind at some position.

   Matthew Gingell
   gingell@adacore.com
*/

#include "chesley.hpp"

// Precompute bitboards for rank attack.
bitboard *
Board::init_rank_attacks_tbl () {
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
	  destinations = 0llu;

	  // Compute moves to the left of from.
	  int k = 0;
	  do
	    {
	      k--;
	      if (from + k < first_bit) break;
	      destinations = set_bit (destinations, from + k);
	    }
	  while (!test_bit (occ, from % 8 + k));

	  // Compute moves to the right of from.
	  k = 0;
	  do
	    {
	      k++;
	      if (from + k > last_bit) break;
	      destinations = set_bit (destinations, from + k);
	    }
	  while (!test_bit (occ, from % 8 + k));

	  rv[from * 256 + occ] = destinations;
	}      
    }

  return rv;
}

// Precompute bitboards for file attack.
bitboard *
Board::init_file_attacks_tbl () {
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

	  destinations = 0llu;

	  // Compute moves up the board.
	  bitnum = from_bit;
	  do
	    {
	      bitnum--;
	      if (bitnum < 0) break;
	      bit_offset = from_bit - bitnum;
	      destinations = 
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
	      destinations = 
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
Board::init_knight_attacks_tbl()
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
	      rv[off] = set_bit (rv[off], nx + ny * 8);
	    }
	}
    }
  return rv;
}

// Precompute bitboards for king moves.
bitboard *
Board::init_king_attacks_tbl()
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
	      rv[off] = set_bit (rv[off], nx + ny * 8);
	    }
	}
    }
  return rv;
}

// Precompute attacks along tht 56 - 07 diagonal.
bitboard *
Board::init_45d_attacks_tbl () {
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
	  destinations = 0llu;

	  // Down and to the right.
	  b = from_bit;
	  do 
	    {
	      b++;
	      if (b >= pat_len) break;
	      destinations = 
		set_bit (destinations, from + (from_bit - b) * 7);
	    } 
	  while (!test_bit(pat, b));

	  // Up and to the left.
	  b = from_bit;
	  do 
	    {
	      b--;
	      if (b < 0) break;
	      destinations = 
		set_bit (destinations, from + (from_bit - b) * 7);
	    } 
	  while (!test_bit(pat, b));

	  rv[from * 256 + pat] = destinations;
	}
    }

  return rv;
}

// Precompute attacks along tht 0 - 63 diagonal.
bitboard *
Board::init_135d_attacks_tbl () {
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
	  destinations = 0llu;

	  // Down and to the left.
	  b = from_bit;
	  do 
	    {
	      b++;
	      if (b >= pat_len) break;
	      destinations = 
		set_bit (destinations, from + (from_bit - b) * 9);
	    } 
	  while (!test_bit(pat, b));

	  // Up and to the left.
	  b = from_bit;
	  do 
	    {
	      b--;
	      if (b < 0) break;
	      destinations = 
		set_bit (destinations, from + (from_bit - b) * 9);
	    } 
	  while (!test_bit(pat, b));

	  rv[from * 256 + pat] = destinations;
	}
    }

  return rv;
}
