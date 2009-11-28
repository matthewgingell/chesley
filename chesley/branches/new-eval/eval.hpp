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

/////////////
// Margins //
/////////////

static const Score LAZY_EVAL_MARGIN = 500;

////////////////////////
// Evaluation weights //
////////////////////////

// Bounds on the Score type.

static const Score INF        = 30 * 1000;
static const Score MATE_VAL   = 20 * 1000;

// Piece values from Larry Kaufman.

static const Score PAWN_VAL   = 100;
static const Score KNIGHT_VAL = 325;
static const Score BISHOP_VAL = 325;
static const Score ROOK_VAL   = 500;
static const Score QUEEN_VAL  = 975;

// Bonuses for castling.

static const Score CAN_CASTLE_K_VAL = 15;
static const Score CAN_CASTLE_Q_VAL = 10;

static const Score CASTLED_KS_VAL = 65;
static const Score CASTLED_QS_VAL = 25;

// Kings on or next to open files.

static const Score KING_ON_OPEN_FILE_VAL = -50;
static const Score KING_NEXT_TO_OPEN_FILE_VAL = -30;

static const Score KING_ON_HALF_OPEN_FILE_VAL = -30;
static const Score KING_NEXT_TO_HALF_OPEN_FILE_VAL = -15;

// King attacked 

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

static const 
Score ISOLATED_VAL[COLOR_COUNT][RANK_COUNT] = {
  { 0, -15,   5,  15,  30,  35,  75,  0 },
  { 0,  75,  35,  30,  15, -15, -15,  0 }
};

static const 
Score BACKWARD_VAL[COLOR_COUNT][RANK_COUNT] = {
  { 0, -15,   5,  15,  30,  50, 100,  0 },
  { 0, 100,  50,  30,  15, -15, -15,  0 }
};

static const 
Score CONNECTED_VAL[COLOR_COUNT][RANK_COUNT] = {
  { 0,   0,  10,  20,  35,  50, 100,  0 },
  { 0, 100,  50,  35,  15,   0,   0,  0 } 
};

static const 
Score PASSED_VAL[COLOR_COUNT][RANK_COUNT] = {
  { 0,   0,   5,  35,  60, 100, 150,   0 },
  { 0, 150, 100,  55,  30,   0,   0,   0 }
};

static const
Score PASSED_CONNECTED_VAL[COLOR_COUNT][RANK_COUNT] = {
  { 0,   0,  25,  60, 130, 250, 300,   0 },
  { 0, 250, 250, 130,  55,   0,   0,   0 }
};

static const 
Score BASE_ADV_VAL[COLOR_COUNT][RANK_COUNT] = {
  { 0, -15, -15,  15,  30,  35,  75,  0 },
  { 0,  75,  35,  30,  15, -15, -15,  0 }  
};

// Knights.

static const Score KNIGHT_OUTPOST_VAL = 25;

// Tempo

static const Score TEMPO_VAL = 10;

// Mobility bonuses
static const Score ROOK_MOBILITY_VAL   = 4;
static const Score KNIGHT_MOBILITY_VAL = 6;
static const Score BISHOP_MOBILITY_VAL = 8;
static const Score QUEEN_MOBILITY_VAL  = 4;

///////////////////////////////
// Inline utility functions. //
///////////////////////////////

// The piece square table.
extern const Score piece_square_table[2][6][64];

// Piece values addressable the piece kind
const Score piece_values[] = 
  { PAWN_VAL, ROOK_VAL, KNIGHT_VAL, BISHOP_VAL, QUEEN_VAL, 0 };

// Return the value of a piece by kind.
inline Score value (Kind k) {
  assert (k >= PAWN && k <= QUEEN_VAL);
  return piece_values[k];
}

// Return the value of a piece captured by a move.
inline Score victim_value (Move m) {  return value (m.capture); }

// Lookup the piece square value of a position.
inline Score piece_square_value (Phase p, Kind k, Color c, Coord idx) {
  Coord off = (c == WHITE) ? idx : flip_white_black[idx];
  return piece_square_table[p][k][off];
}

// Lookup the change in piece square value over a move.
inline Score piece_square_value (const Board &b, const Move &m) {
#if 0
  Color c = b.to_move();
  Coord f = m.from;
  Coord t = m.to;
  Kind k = m.get_kind();

  Score p1 = piece_square_table[OPENING_PHASE][k][xfrm[c][to]] -
    piece_square_table[OPENING_PHASE][k][xfrm[c][from]];

  Score p2 = piece_square_table[END_PHASE][k][xfrm[c][to]] -
    piece_square_table[END_PHASE][k][xfrm[c][from]];

  return interpolate (p1, p2);
#endif 
  return 0;
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

  Score s;
  Score s_op;
  Score s_eg;

  bool open_file[FILE_COUNT];
  bool half_open_file[FILE_COUNT];

  bitboard attack_set[COLOR_COUNT];
  
  ///////////////////////////////
  // Static feature evaluation //
  ///////////////////////////////

  void compute_features ();

  Score score_king             (const Color c);
  Score score_knight           (const Color c);
  Score score_bishop           (const Color c);
  Score score_mobility         (const Color c);
  Score score_rooks_and_queens (const Color c);

  Score score_pawns            ();
  Score score_pawns_inner      (const Color c);

  ///////////////
  // Utilities //
  ///////////////

  // Interpolate between opening and end game values.
  Score interpolate (Score s_op, Score s_eg);

  ///////////////////
  // Debug methods //
  ///////////////////

  Score sum_net_material ();
};

#endif // _EVAL_
