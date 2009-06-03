////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// eval.cpp                                                                   //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <iostream>

using namespace std;

#include "chesley.hpp"


///////////////////////////////////////////////////////////////////////
//                                                                   //
// centrality_table:                                                 //
//                                                                   //
// This is a table reflecting the relative value of locations on the //
// chess board based on their proximity to the center.               //
//                                                                   //
///////////////////////////////////////////////////////////////////////

int8 const Eval::centrality_table[64] =
  {
     1,   1,   1,   1,   1,   1,   1,   1,
     1,   1,   1,   1,   1,   1,   1,   1,
     1,   1,   2,   2,   2,   2,   1,   1,
     1,   1,   2,   4,   4,   2,   1,   1,
     1,   1,   2,   4,   4,   2,   1,   1,
     1,   1,   2,   2,   2,   2,   1,   1,
     1,   1,   1,   1,   1,   1,   1,   1,
     1,   1,   1,   1,   1,   1,   1,   1
  };

////////////////////////////////////////////////////////////////////////////
//                                                                        //
//  piece_square_table:                                                   //
//                                                                        //
//  This is a table of bonuses for each piece-location pair. The table    //
//  is written in 'reverse' for readability and a transformation is       //
//  required for fetching values for black and white.                     //
//                                                                        //
////////////////////////////////////////////////////////////////////////////

static const int8 piece_square_table[6][64] =
{
  // Pawns
  {
     0,   0,   0,   0,   0,   0,   0,   0,
    70,  70,  70,  70,  70,  70,  70,  70,
    10,  10,  20,  30,  30,  20,  10,  10,
     5,   5,  10,  16,  16,  10,   5,   5,
     3,   3,   3,  13,  13,   3,   3,   3,
     1,   1,   1,  10,  10,   1,   1,   1,
     0,   0,   0, -12, -12,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0
  },

  // Rooks
  {
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0
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
   -10,   0,   5,   5,  5,   5,   0, -10,
   -10,   0,   0,   0,  0,   0,   0, -10,
   -20, -10, -10,  -5, -5, -10, -10, -20
  },

  // Kings
  {
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0
  }
};

// Evaluate a positional strength based on the preceding table.
Score
sum_piece_squares (const Board &b) {
  Score bonus = 0;
  for (Color c = WHITE; c <= BLACK; c++)
    {
      bitboard all_color_pieces = b.color_to_board (c);
      for (Kind k = PAWN; k <= KING; k++)
        {
          bitboard pieces = all_color_pieces & b.kind_to_board (k);
          while (pieces)
            {
              int idx = bit_idx (pieces);
              int xfrm = (c == WHITE) ? 63 - idx : idx;
              bonus += sign (c) * piece_square_table[k][xfrm];
              pieces = clear_lsb (pieces);
            }
        }
    }
  return bonus;
}
