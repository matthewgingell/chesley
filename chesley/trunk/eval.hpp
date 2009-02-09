/*
  Here we define a function for evaluating the strength of a position
  heuristically.

  Matthew Gingell
  gingell@adacore.com
*/

#ifndef _EVAL_
#define _EVAL_

#include "bits64.hpp"
#include "board.hpp"
#include "util.hpp"

// Material values.

static const int INFINITY   = 1000 * 1000 * 1000;

static const int QUEEN_VAL  = 900;
static const int ROOK_VAL   = 500;
static const int BISHOP_VAL = 300;
static const int KNIGHT_VAL = 300;
static const int PAWN_VAL   = 100;

static const int MATE_VAL   = 1000 * 1000;

// Positional values of having castled and retaining the right to
// castle.
static const int KS_CASTLE_VAL  = 75;
static const int QS_CASTLE_VAL  = 50;
static const int CAN_CASTLE_VAL = 10;

// Simple table driven positional bonuses.
int 
eval_simple_positional (Board b);

inline int 
eval_material (const Board &b) {
  int score = 0;

  /*******************************/
  /* Evaluate material strength. */
  /*******************************/

  score += PAWN_VAL * (count_bits (b.pawns & b.white) - 
		       count_bits (b.pawns & b.black));
  
  score += ROOK_VAL * (count_bits (b.rooks & b.white) - 
		       count_bits (b.rooks & b.black));
  
  score += KNIGHT_VAL * (count_bits (b.knights & b.white) - 
			 count_bits (b.knights & b.black));
  
  score += BISHOP_VAL * (count_bits (b.bishops & b.white) - 
			 count_bits (b.bishops & b.black));
  
  score += QUEEN_VAL * (count_bits (b.queens & b.white) - 
			count_bits (b.queens & b.black));

  return score;
}

// Evaluate a position statically. Positive scores favor white and
// negative scores favor black.
inline int 
eval (const Board &b, int depth = 0) {
  int score = 0;

  score += eval_material (b);

  /***************************/
  /* Evaluate pawn structure */
  /***************************/

  // ????

  /*********************************/
  /* Evaluate positional strength. */
  /*********************************/

  // Do table driven adjustments for individual pieces. Each (color,
  // piece, location) tuple is assigned a static value and we sum over
  // the board to obtain an estimate of the value of this position.

  score += eval_simple_positional (b);

  // Encourage preserving the right to castle.
  score += CAN_CASTLE_VAL * (b.flags.w_can_q_castle - b.flags.b_can_q_castle);
  score += CAN_CASTLE_VAL * (b.flags.w_can_k_castle - b.flags.b_can_k_castle);

  // Encourage castling, and prefer king side to queen side.
  score += KS_CASTLE_VAL * (b.flags.w_has_k_castled - b.flags.b_has_k_castled);
  score += QS_CASTLE_VAL * (b.flags.w_has_q_castled - b.flags.b_has_q_castled);
  
  // All else being equal, prefer positions at a shallower rather than
  // a deeper depth. If we don't do this, minimax will be ambivalent
  // between winning in 1 move and 2 moves *at every ply* and may
  // never converge!!!

  score += depth * -b.flags.to_move;
    
  return score;
}

#endif // _EVAL_
