////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// eval.hpp                                                                   //
//                                                                            //
// Here we define a function for evaluating the strength of a position        //
// heuristically. Internally, the convention is that scores favoring          //
// white are positive and those for black are negative. However, scores       //
// returned to the player are multiplied by the correct sign and are          //
// appropriate for maximization.                                              //
//                                                                            //
// The starting point for the approach taken here is Tomasz                   //
// Michniewski's proposal for "Unified Evaluation" tournements. The           //
// full discussion of that very simple scoring strategy is avaiable at:       //
//                                                                            //
// http://chessprogramming.wikispaces.com/simplified+evaluation+function      //
//                                                                            //
// Matthew Gingell                                                            //
// gingell@adacore.com                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _EVAL_
#define _EVAL_

#include <iostream>

#include "chesley.hpp"

//////////////////////
// Material values. //
//////////////////////

static const Score PAWN_VAL   = 100;
static const Score ROOK_VAL   = 500;
static const Score KNIGHT_VAL = 300;
static const Score BISHOP_VAL = 325;
static const Score QUEEN_VAL  = 975;

static const Score MATE_VAL   = 500  * 1000;
static const Score INF        = 1000 * 1000;

inline Score eval_piece (Kind k) {
  switch (k)
    {
    case PAWN:   return PAWN_VAL;
    case ROOK:   return ROOK_VAL;
    case KNIGHT: return KNIGHT_VAL;
    case BISHOP: return BISHOP_VAL;
    case QUEEN:  return QUEEN_VAL;
    default:     return 0;
    }
}

///////////////////////
// Castling bonuses. //
///////////////////////

static const Score KS_CASTLE_VAL  = 75;
static const Score QS_CASTLE_VAL  = 50;
static const Score CAN_CASTLE_VAL = 10;

///////////////////////////
// Evaluation functions. //
///////////////////////////

// Compute the net material value for this position.
inline Score eval_material (const Board &b);

// Compute a simple net positional value from the piece square table.
Score sum_piece_squares (const Board &b);

// Compute a score for this position.
inline Score
eval (const Board &b) {
  Score score = 0;

  ///////////////////////
  // Evaluate material //
  ///////////////////////

  score += eval_material (b);

  // A bishop pair is worth an extra 1/2 pawn.
  // A pawn and the A or H file is only worth 3/4 of a pawn
  // Trading material is good when you are ahead.

  // The fewer pieces on the board, the fewer pawns a minor piece is
  // worth.

  ///////////////////////
  // Evaluate mobility //
  ///////////////////////

  // ??????????

  /////////////////////////////
  // Evaluate pawn structure //
  /////////////////////////////

  // ??????????

  ///////////////////////////////////
  // Evaluate positional strength. //
  ///////////////////////////////////

#if 0
  Board c = b;
  c.set_color (invert_color (c.to_move ()));
  assert (sum_piece_squares (b) == sum_piece_squares (c));
#endif // NDEBUG
  
  score += sum_piece_squares (b);

  ////////////////////////
  // Evaluate castling. //
  ////////////////////////

  // ????????????

#if 0
  /////////////////////////////////////////////////
  // Add some random noise for variety of games. // 
  /////////////////////////////////////////////////

  score += random () % 5;
#endif

  //////////////////////////////////////////////////
  // Return appropriately signed score to caller. //
  //////////////////////////////////////////////////

  return sign (b.to_move ()) * score;
}

// Compute the net material value for this position.
inline Score
eval_material (const Board &b) {
  Score score = 0;

  score += PAWN_VAL * (pop_count (b.pawns & b.white) -
		       pop_count (b.pawns & b.black));

  // Pawns on the A and H files are penalized a quarter pawn.
  score -= pop_count (b.pawns & b.white & (b.file (0) || b.file (7))) * 25;
  score += pop_count (b.pawns & b.black & (b.file (0) || b.file (7))) * 25;

  score += ROOK_VAL * (pop_count (b.rooks & b.white) -
		       pop_count (b.rooks & b.black));

  score += KNIGHT_VAL * (pop_count (b.knights & b.white) -
			 pop_count (b.knights & b.black));

  // A bishop pair is awarded a 1/2 pawn bonus.
  int nbishops_white = pop_count (b.bishops & b.white);
  int nbishops_black = pop_count (b.bishops & b.black);
  if (nbishops_white == 2) score += 50;
  if (nbishops_black == 2) score -= 50;
  score += BISHOP_VAL * (nbishops_white - nbishops_black);

  score += QUEEN_VAL * (pop_count (b.queens & b.white) -
			pop_count (b.queens & b.black));

  return score;
}

#endif // _EVAL_
