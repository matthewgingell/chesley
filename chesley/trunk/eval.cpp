/*
  eval.cpp

  Matthew Gingell
  gingell@adacore.com
*/

#include "chesley.hpp"

/***************************************************************************/
/*  piece_square_table:                                                   */
/*                                                                        */
/*  This is a table of bonuses for each piece-location pair. The table    */
/*  is written in 'reverse' for readablility and a transformation is      */
/*  required for fetching values for black and white.                     */
/*                                                                        */
/*  The values in the following table are taken from Tomasz               */
/*  Michniewski's proposal for "Unified Evaluation" tournaments. The      */
/*  full discussion of that very simple scoring strategy is available     */
/*  at:                                                                   */
/*                                                                        */
/*  http://chessprogramming.wikispaces.com/simplified+evaluation+function */
/**************************************************************************/

// Bonuses for white indexed by [piece][64 - square].
//
// Transformation for white is idx => (56 - 8 * (x / 8) + x % 8)
// Transformation for black is idx => 63 - (56 - 8 * (x / 8) + x % 8)

static int piece_square_table[6][64] =
{
  // Pawns
  {   
     0,   0,   0,   0,   0,   0,   0,   0,
    50,  50,  50,  50,  50,  50,  50,  50,
    10,  10,  20,  30,  30,  20,  10,  10,
     5,   5,  10,  25,  25,  10,   5,   5,
     0,   0,   0,  20,  20,   0,   0,   0,
     5,  -5, -10,   0,   0, -10,  -5,   5,
     5,  10,  10, -20, -20,  10,  10,   5,
     0,   0,   0,   0,   0,   0,   0,   0 
  },

  // Rooks
  { 
     0,   0,   0,   0,   0,   0,   0,   0,
     5,  10,  10,  10,  10,  10,  10,   5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
     0,   3,   4,   5,   5,   4,   3,   0
  },

  // Knights
  { 
   -50, -40, -30, -30, -30, -30, -40, -50,
   -40, -20,   0,   0,   0,   0, -20, -40,
   -30,   0,  10,  15,  15,  10,   0, -30,
   -30,   5,  15,  20,  20,  15,   5, -30,
   -30,   0,  15,  20,  20,  15,   0, -30,
   -30,   5,  10,  15,  15,  10,   5, -30,
   -40, -20,   0,   5,   5,   0, -20, -40,
   -50, -40, -30, -30, -30, -30, -40, -50 
  },
  
  // Bishops
  { 
   -20, -10, -10, -10, -10, -10, -10, -20,
   -10,   0,   0,   0,   0,   0,   0, -10,
   -10,   0,   5,  10,  10,   5,   0, -10,
   -10,   5,   5,  10,  10,   5,   5, -10,
   -10,   0,  10,  10,  10,  10,   0, -10,
   -10,  10,  10,  10,  10,  10,  10, -10,
   -10,   5,   0,   0,   0,   0,   5, -10,
   -20, -10, -10, -10, -10, -10, -10, -20 
  },

  // Queens
  { 
   -20, -10, -10,  -5, -5, -10, -10, -20,
   -10,   0,   0,   0,  0,   0,   0, -10,
   -10,   0,   5,   5,  5,   5,   0, -10,
    -5,   0,   5,   5,  5,   5,   0,  -5,
     0,   0,   5,   5,  5,   5,   0,  -5,
   -10,   5,   5,   5,  5,   5,   0, -10,
   -10,   0,   5,   0,  0,   0,   0, -10,
   -20, -10, -10,  -5, -5, -10, -10, -20 
  },

  // Kings
  {
   -30, -40, -40, -50, -50, -40, -40, -30,
   -30, -40, -40, -50, -50, -40, -40, -30,
   -30, -40, -40, -50, -50, -40, -40, -30,
   -30, -40, -40, -50, -50, -40, -40, -30,
   -20, -30, -30, -40, -40, -30, -30, -20,
   -10, -20, -20, -20, -20, -20, -20, -10,
    20,  20,   0,   0,   0,   0,  20,  20,
    20,  30,  10,   0,   0,  10,  30,  20 
  }
};

#if 0

// king end game
-50,-40,-30,-20,-20,-30,-40,-50,
-30,-20,-10,  0,  0,-10,-20,-30,
-30,-10, 20, 30, 30, 20,-10,-30,
-30,-10, 30, 40, 40, 30,-10,-30,
-30,-10, 30, 40, 40, 30,-10,-30,
-30,-10, 20, 30, 30, 20,-10,-30,
-30,-30,  0,  0,  0,  0,-30,-30,
-50,-30,-30,-30,-30,-30,-30,-50

#endif

// Evaluate a the positional strength of a based based on the
// preceding table.
Score
eval_piece_squares (const Board &b) {
  Score bonus = 0;

  // Sum over all colors and kinds.
  for (int c = WHITE; c <= BLACK; c++)
    for (int k = PAWN; k <= KING; k++)
      {
	bitboard pieces =  
	  b.color_to_board ((Color) c) &  b.kind_to_board ((Kind) k);

	while (pieces)
	  {
	    int idx = bit_idx (pieces);
	    int xfrm = 56 - 8 * (idx / 8) + idx % 8;
	    if (c == BLACK) xfrm = 63 - xfrm;
	    bonus += sign ((Color) c) * piece_square_table[k][xfrm];
	    pieces = clear_lsb (pieces);
	  }
    }

  return bonus;
}
