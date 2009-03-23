/*
  Here we define a function for evaluating the strength of a position
  heuristically.

  Matthew Gingell
  gingell@adacore.com
*/

#ifndef _EVAL_
#define _EVAL_

#include <cstdlib>
#include <iostream>
#include "bits64.hpp"
#include "board.hpp"
#include "types.hpp"
#include "util.hpp"

// Material values.

static const Score INF        = 1000 * 1000;

static const Score QUEEN_VAL  = 900;
static const Score ROOK_VAL   = 500;
static const Score BISHOP_VAL = 300;
static const Score KNIGHT_VAL = 300;
static const Score PAWN_VAL   = 100;

static const Score MATE_VAL   = 500 * 1000;
static const Score CONTEMPT_VAL   = 1000;

// Positional values of having castled and retaining the right to
// castle.
static const Score KS_CASTLE_VAL  = 75;
static const Score QS_CASTLE_VAL  = 50;
static const Score CAN_CASTLE_VAL = 10;

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

// Simple table driven positional bonuses.
Score eval_simple_positional (const Board &b);

inline Score
eval_material (const Board &b) {
  Score score = 0;

  /*******************************/
  /* Evaluate material strength. */
  /*******************************/

  score += PAWN_VAL * (pop_count (b.pawns & b.white) -
		       pop_count (b.pawns & b.black));

  score += ROOK_VAL * (pop_count (b.rooks & b.white) -
		       pop_count (b.rooks & b.black));

  score += KNIGHT_VAL * (pop_count (b.knights & b.white) -
			 pop_count (b.knights & b.black));

  score += BISHOP_VAL * (pop_count (b.bishops & b.white) -
			 pop_count (b.bishops & b.black));

  score += QUEEN_VAL * (pop_count (b.queens & b.white) -
			pop_count (b.queens & b.black));

  return score;
}

// Evaluate a position statically. Positive scores favor white and
// negative scores favor black.
inline Score
eval (const Board &b) {
  Score score = 0;

  if (b.half_move_clock == 50) return 0;

  score += eval_material (b);

  /***************************/
  /* Evaluate pawn structure */
  /***************************/

  // ????

  /*********************************/
  /* Evaluate positional strength. */
  /*********************************/

  score += eval_simple_positional (b);

  // Encourage preserving the right to castle.
  score += CAN_CASTLE_VAL * (b.flags.w_can_q_castle - b.flags.b_can_q_castle);
  score += CAN_CASTLE_VAL * (b.flags.w_can_k_castle - b.flags.b_can_k_castle);

  // Encourage castling, and prefer king side to queen side.
  score += KS_CASTLE_VAL * (b.flags.w_has_k_castled - b.flags.b_has_k_castled);
  score += QS_CASTLE_VAL * (b.flags.w_has_q_castled - b.flags.b_has_q_castled);

#if 0
  score += random () % 10;
#endif

  return sign (b.flags.to_move) * score;
}

#endif // _EVAL_
