////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// weights.hpp                                                                //
//                                                                            //
// Here we define various weights and tables for static evaluation.           //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _WEIGHTS_
#define _WEIGHTS_

/////////////
// Margins //
/////////////

static const Score LAZY_EVAL_MARGIN = 300;

////////////////////////
// Evaluation weights //
////////////////////////

// Bonuses for castling.

static const Score CAN_CASTLE_K_VAL = 15;
static const Score CAN_CASTLE_Q_VAL = 10;

static const Score CASTLED_KS_VAL = 65;
static const Score CASTLED_QS_VAL = 25;

// Kings on or next to an open file.

static const Score KING_ON_OPEN_FILE_VAL = -50;
static const Score KING_NEXT_TO_OPEN_FILE_VAL = -30;

static const Score KING_ON_HALF_OPEN_FILE_VAL = -30;
static const Score KING_NEXT_TO_HALF_OPEN_FILE_VAL = -15;

// A square adjacent to the king is attacked.

static const Score KING_ADJACENT_ATTACKED_VAL = -25;

// Bonuses for rooks and queens.

static const Score ROOK_OPEN_FILE_VALUE = 50;
static const Score QUEEN_OPEN_FILE_VALUE = 50;

static const Score ROOK_OPEN_VAL = 40;
static const Score ROOK_HALF_VAL = 20;
static const Score QUEEN_OPEN_VAL = 20;
static const Score QUEEN_HALF_VAL = 10;

static const Score ROOK_ON_7TH_VAL = 50;

// Bishops
static const Score BISHOP_TRAPPED_A7H7 = 150;
static const Score BISHOP_TRAPPED_A6H6 =  75;

// Bishop pair value from Larry Kaufman.
static const Score BISHOP_PAIR_VAL = 50;

///////////////////////////////////////////////////////////////////
// Pawn structure bonuses based on Hans Berliner's _The System_. //
///////////////////////////////////////////////////////////////////

// Pawn structure adjustments based on feature computed during
// evaluation.

static const Score WEAK_PAWN_VAL = 20;

#define pair(a, b, c, d, e, f, g, h) \
  {{ (a), (b), (c), (d), (e), (f), (g), (h) }, \
   { (h), (g), (f), (e), (d), (c), (b), (a) } }

static const 
Score CONNECTED_VAL[COLOR_COUNT][RANK_COUNT] = 
  pair(    0,   0,  10,  20,  35,  50, 100,   0);

static const 
Score PASSED_VAL[COLOR_COUNT][RANK_COUNT] =
  pair(    0,  10,  20,  50,  75, 125, 150,   0);

static const
Score PASSED_CONNECTED_VAL[COLOR_COUNT][RANK_COUNT] =
  pair(    0,  10,  30,  60, 100, 150, 250,   0);

#undef pair

// Knights.

static const Score KNIGHT_OUTPOST_VAL = 25;

// Tempo

static const Score TEMPO_VAL = 10;

// Mobility bonuses
static const Score PAWN_MOBILITY_VAL   = 5;
static const Score ROOK_MOBILITY_VAL   = 5;
static const Score KNIGHT_MOBILITY_VAL = 6;
static const Score BISHOP_MOBILITY_VAL = 8;
static const Score QUEEN_MOBILITY_VAL  = 4;

// King safety
static const Score PAWN_SHEILD_1_VAL  = 10;
static const Score PAWN_SHEILD_2_VAL  =  5;

/////////////////////////////////////////////////////////////////////////
//                                                                     //
//  piece_square_table:                                                //
//                                                                     //
//  This is a table of bonuses for each piece-location pair. The table //
//  is written in reverse for readability and a transformation is      //
//  required for fetching values for black and white.                  //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

const Score piece_square_table[2][6][64] =
  {
    // Values used in the opening.
    {

      // Pawn values based on Hans Berliner.
      {
         0,   0,   0,   0,   0,   0,   0,   0,
         6,  12,  25,  50,  50,  25,  12,   6,
         6,  12,  25,  50,  50,  25,  12,   6,
        -3,   3,  17,  28,  28,  17,   3,  -3,
       -10,  -5,  10,  20,  20,  10,  -5, -10,
       -10,  -5,   5,  15,  15,   5,  -5, -10,
       -10,  -5,   5, -10, -10,   5,  -5, -10,
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
       -50, -20, -20, -10, -10, -20, -20, -50,
       -20,  15,  15,  25,  25,  15,  15, -20,
       -10,  15,  20,  25,  25,  20,   0, -10,
         0,  10,  20,  25,  25,  20,  10,   0,
         0,  10,  15,  20,  20,  15,  10,   0,
         0,   0,  15,  10,  10,  15,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,
       -50, -20, -20, -20, -20, -20, -20, -50
      },

      // Bishops
      {
         0,   0,   0,   5,   5,   0,   0,   0,
         0,   5,   5,   5,   5,   5,   5,   0,
         0,   5,  10,  10,  10,  10,   5,   0,
         0,   5,  10,  15,  15,  10,   5,   0,
         0,   5,  10,  15,  15,  10,   5,   0,
         0,   5,  10,  10,  10,  10,   5,   0,
         0,   5,   5,   5,   5,   5,   5,   0,
       -10, -10, -10,  -5,  -5,  10,  10, -10
      },

      // Queens
      {
         0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,  25,  50,  50,  25,   0,   0,
         0,   0,  50,  75,  75,  50,   0,   0
      },

      // Kings
      {
        -40, -40, -40, -40, -40, -40, -40, -40,
        -40, -40, -40, -40, -40, -40, -40, -40,
        -40, -40, -40, -40, -40, -40, -40, -40,
        -40, -40, -40, -40, -40, -40, -40, -40,
        -40, -40, -40, -40, -40, -40, -40, -40,
        -40, -40, -40, -40, -40, -40, -40, -40,
        -20, -20, -20, -20, -20, -20, -20, -20,
          0,  20,  40, -20,   0, -20,  40,  20
      }

    },

    // Values used in the end game.
    {
      // Pawn values based on Hans Berliner.
      {
#define eg(x) x
        eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg( 45), eg( 29), eg( 16), eg(  5), eg(  5), eg( 16), eg( 29), eg( 45),
        eg( 45), eg( 29), eg( 16), eg(  5), eg(  5), eg( 16), eg( 29), eg( 45),
        eg( 33), eg( 17), eg(  7), eg(  1), eg(  1), eg(  7), eg( 17), eg( 33),
        eg( 25), eg( 10), eg(  0), eg( -5), eg( -5), eg(  0), eg( 10), eg( 25),
        eg( 20), eg(  5), eg( -5), eg(-10), eg(-10), eg( -5), eg(  5), eg( 20),
        eg( 20), eg(  5), eg( -5), eg(-10), eg(-10), eg( -5), eg(  5), eg( 20),
        eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0)
#undef eg
      },
      
      // Rooks
      {
#define eg(x) (x - 25) // Why??????
        eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0)
#undef eg
      },

      // Knights
#define eg(x) (x - 25)
      {
        eg(-50), eg (-20), eg(-20), eg(-10), eg(-10), eg(-20), eg(-20), eg(-50),
        eg(-20), eg ( 15), eg( 15), eg( 25), eg( 25), eg( 15), eg( 15), eg(-20),
        eg(-10), eg ( 15), eg( 20), eg( 25), eg( 25), eg( 20), eg(  0), eg(-10),
        eg(  0), eg ( 10), eg( 20), eg( 25), eg( 25), eg( 20), eg( 10), eg(  0),
        eg(  0), eg ( 10), eg( 15), eg( 20), eg( 20), eg( 15), eg( 10), eg(  0),
        eg(  0), eg (  0), eg( 15), eg( 10), eg( 10), eg( 15), eg(  0), eg(  0),
        eg(  0), eg (  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(-50), eg (-20), eg(-20), eg(-20), eg(-20), eg(-20), eg(-20), eg(-50)
#undef eg
      },

      // Bishops
#define eg(x) (x + 25)
      {
        eg(  0), eg (  0), eg(  0), eg(  5), eg(  5), eg(  0), eg(  0), eg(  0),
        eg(  0), eg (  5), eg(  5), eg(  5), eg(  5), eg(  5), eg(  5), eg(  0),
        eg(  0), eg (  5), eg( 10), eg( 10), eg( 10), eg( 10), eg(  5), eg(  0),
        eg(  0), eg (  5), eg( 10), eg( 15), eg( 15), eg( 10), eg(  5), eg(  0),
        eg(  0), eg (  5), eg( 10), eg( 15), eg( 15), eg( 10), eg(  5), eg(  0),
        eg(  0), eg (  5), eg( 10), eg( 10), eg( 10), eg( 10), eg(  5), eg(  0),
        eg(  0), eg (  5), eg(  5), eg(  5), eg(  5), eg(  5), eg(  5), eg(  0),
        eg(-10), eg (-10), eg(-10), eg( -5), eg( -5), eg( 10), eg( 10), eg(-10)
#undef eg
      },

      // Queens
      {
#define eg(x) (x)
        eg(  0), eg (  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg (  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg (  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg (  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg (  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg (  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg (  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg (  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0)
#undef eg
      },

      // Kings
      {
#define eg(x) (x)
        eg(  0), eg ( 10), eg( 20), eg( 30), eg( 30), eg( 20), eg( 10), eg(  0),
        eg( 10), eg ( 20), eg( 30), eg( 40), eg( 40), eg( 30), eg( 20), eg( 10),
        eg( 20), eg ( 30), eg( 40), eg( 50), eg( 50), eg( 40), eg( 30), eg( 20),
        eg( 30), eg ( 40), eg( 50), eg( 60), eg( 60), eg( 50), eg( 40), eg( 30),
        eg( 30), eg ( 40), eg( 50), eg( 60), eg( 60), eg( 50), eg( 40), eg( 30),
        eg( 20), eg ( 30), eg( 40), eg( 50), eg( 50), eg( 40), eg( 30), eg( 20),
        eg( 10), eg ( 20), eg( 30), eg( 40), eg( 40), eg( 30), eg( 20), eg( 10),
        eg(  0), eg ( 10), eg( 20), eg( 30), eg( 30), eg( 20), eg( 10), eg(  0)
#undef eg
      }
    }
  };

#endif // _WEIGHTS_
