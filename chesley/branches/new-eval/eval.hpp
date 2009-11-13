////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// eval.hpp                                                                   //
//                                                                            //
// Here we define a function for evaluating the strength of a position        //
// heuristically. Internally, the convention is that scores favoring          //
// white are positive and those for black are negative. However, scores       //
// returned to the player are multiplied by the correct sign and are          //
// appropriate for negamax.                                                   //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _EVAL_
#define _EVAL_

#include "chesley.hpp"

static const Score INF        = 30 * 1000;
static const Score MATE_VAL   = 20 * 1000;

// Piece values from Larry Kaufman.

static const Score PAWN_VAL   = 100;
static const Score KNIGHT_VAL = 325;
static const Score BISHOP_VAL = 325;
static const Score ROOK_VAL   = 500;
static const Score QUEEN_VAL  = 975;

// Position evaluation type.

struct Eval {

  Eval (const Board &b, Score alpha = -INF, Score beta = -INF) :
    b (b) {
    alpha = beta;
  }

  Score score ();

private:

  const Board &b;

  Score score_mobility (const Color c);
};

///////////////////////////////
// Inline utility functions. //
///////////////////////////////

const Score piece_values[] = 
  { PAWN_VAL, ROOK_VAL, KNIGHT_VAL, BISHOP_VAL, QUEEN_VAL, INF };

inline Score 
value (Kind k) {
  assert (k >= PAWN && k <= QUEEN_VAL);
  return piece_values[k];
}

inline Score 
victim_value (Move m) {
  return value (m.capture);
}

extern const Score piece_square_table[2][6][64];
extern const int xfrm[2][64];

inline Score 
piece_square_value (Kind k, Color c, Coord idx) {
  return piece_square_table[OPENING_PHASE][k][xfrm[c][idx]];
}

inline Score 
piece_square_value (const Board &b, const Move &m) {
  return piece_square_table[m.get_kind()][xfrm[b.to_move()][m.to]] -
    piece_square_table[m.get_kind()][xfrm[b.to_move()][m.from]];
}

inline 
Score net_material (const Board &b) {
  return b.material[WHITE] - b.material[BLACK];
}

#endif // _EVAL_
