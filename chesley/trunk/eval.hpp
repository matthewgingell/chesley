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

// Bounds on the Score type.

static const Score INF        = 30 * 1000;
static const Score MATE_VAL   = 20 * 1000;

// Piece values from Larry Kaufman.

static const Score PAWN_VAL   = 100;
static const Score KNIGHT_VAL = 325;
static const Score BISHOP_VAL = 325;
static const Score ROOK_VAL   = 500;
static const Score QUEEN_VAL  = 975;
static const Score KING_VAL   = 0;
  
const Score max_material = 2 * 
  (8 * PAWN_VAL + 2 * (ROOK_VAL + KNIGHT_VAL + BISHOP_VAL) + QUEEN_VAL);

///////////////////////////////
// Inline utility functions. //
///////////////////////////////

// The piece square table.
extern const Score piece_square_table[2][6][64];

// Piece values addressable the piece kind
const Score piece_values[] = 
  { PAWN_VAL, ROOK_VAL, KNIGHT_VAL, BISHOP_VAL, QUEEN_VAL, KING_VAL };

// Return the value of a piece by kind.
inline Score value (Kind k) {
  assert (k >= PAWN && k <= KING);
  return piece_values[k];
}

// Return the value of a piece captured by a move.
inline Score victim_value (Move m) {  return value (m.capture); }

// Return the value of the piece moving.
inline Score attacker_value (Move m) { return value (m.kind); }

// Interpolate between opening and end game values.
inline Score interpolate (const Board &b, Score s_op, Score s_eg) {
  const Score total_material = b.material[WHITE] + b.material[BLACK];
  return (total_material * s_op + (max_material - total_material) * s_eg) 
    / max_material;
}

// Lookup the piece square value of a position.
inline Score piece_square_value (Phase p, Kind k, Color c, Coord idx) {
  Coord off = (c == BLACK) ? idx : flip_white_black[idx];
  return piece_square_table[p][k][off];
}

// Lookup the interpolated piece square value of a position.
inline Score
interpolated_psq_val (const Board &b, Kind k, Color c, Coord idx) {
  return interpolate 
    (b,
     piece_square_value (OPENING_PHASE, k, c, idx),
     piece_square_value (END_PHASE, k, c, idx));
}

// Lookup the change in piece square value over a move.
inline Score piece_square_value (const Board &b, const Move &m) {
  return 0;

  return interpolated_psq_val (b, m.kind, m.color, m.to) -
    interpolated_psq_val (b, m.kind, m.color, m.from);
}

// Return the net material value of a position.
inline Score net_material (const Board &b) {
  return sign (b.to_move ()) * (b.material[WHITE] - b.material[BLACK]);
}

//////////////////////////////
// Position evaluation type //
//////////////////////////////

struct Eval {

  // Initialize the evaluation object.
  Eval (const Board &b, Score alpha = -INF, Score beta = -INF) :
    b (b), alpha (alpha), beta (beta), s (0), s_op (0), s_eg (0) {}
  
  // Return the static evaluation of this position.
  Score score ();

private:

  const Board &b;
  const Score alpha;
  const Score beta;

  Score total_material;

  Score s;
  Score s_op;
  Score s_eg;

  bool open_file[FILE_COUNT];
  bool half_open_file[FILE_COUNT];

  int pawn_count[COLOR_COUNT];
  int major_count[COLOR_COUNT];
  int minor_count[COLOR_COUNT];

  bitboard attack_set[COLOR_COUNT];
  
  ///////////////////////////////
  // Static feature evaluation //
  ///////////////////////////////

  void compute_features ();

  bool can_not_win             (Color c);
  bool is_draw                 ();

  Score score_king             (const Color c);
  Score score_knight           (const Color c);
  Score score_bishop           (const Color c);
  Score score_mobility         (const Color c);
  Score score_rooks_and_queens (const Color c);

  Score score_pawns            ();
  Score score_pawns_inner      (const Color c);

  ///////////////////
  // Debug methods //
  ///////////////////

  Score sum_net_material ();
};

#endif // _EVAL_
