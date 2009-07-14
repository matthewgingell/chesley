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
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _EVAL_
#define _EVAL_

#include "chesley.hpp"

////////////////////////////////////////////////
// Material and feature evaluation constants. //
////////////////////////////////////////////////

static const Score PAWN_VAL   = 100;
static const Score ROOK_VAL   = 500;
static const Score KNIGHT_VAL = 300;
static const Score BISHOP_VAL = 325;
static const Score QUEEN_VAL  = 975;

static const Score MATE_VAL   = 500  * 1000;
static const Score INF        = 1000 * 1000;

/////////////////////////
// The evaluator type. //
/////////////////////////

struct Eval {

  ////////////////////
  // Static tables. //
  ////////////////////
  
  // The main piece square table.
  static const int8 piece_square_table[6][64];

  // A table for computing indicies from the perspective of one or the
  // other color.
  static const int8 xfrm[2][64];

  // A table reflecting centrality values.
  static const int8 centrality_table[64];

  // A table used for computing the positional value of the king.
  static const int8 king_square_table[3][64];

  /////////////////////
  // Initialization. //
  /////////////////////

  Eval (const Board &board) {
    b = board;
    memset (piece_counts, sizeof (piece_counts), 0);
    memset (minor_counts, sizeof (minor_counts), 0);
    memset (major_counts, sizeof (major_counts), 0);
    memset (pawn_counts,  sizeof (pawn_counts), 0);
    count_material ();
  }

  // Calculate a score from a material imbalance.
  Score net_material () {
    return sign (b.to_move ()) * (b.material[WHITE] - b.material[BLACK]);
  }
  
  /////////////////////////////////
  // Static position evaluation. //
  /////////////////////////////////

  // Top level evaluation function.
  Score score ();

  // Evaluate mobiilty by piece kind.
  Score eval_mobility (const Color c);

  // Evaluate by piece kind.
  Score eval_pawns   (const Color c);
  Score eval_rooks   (const Color c);
  Score eval_knights (const Color c);
  Score eval_bishops (const Color c);
  Score eval_queens  (const Color c);
  Score eval_king    (const Color c);

  // Draw detection.
  bool is_draw ();

  // Initialize piece counts.
  void count_material ();

  // Sum piece table values over white and black pieces. This is only
  // provided for debuging purposes as piece square sums are computed
  // incrementally and saved in the Board data structure.
  Score sum_piece_squares (const Board &b);

  ////////////////////////////////
  // Inline utility functions.  //
  ////////////////////////////////

  // Fetch the piece square table value for a piece.
  static inline Score 
  psq_value (Kind k, Color c, coord idx) {
    return piece_square_table[k][xfrm[c][idx]];
  }
  
  // Fetch a net score for a move from the piece square tables.
  static inline Score 
  psq_value (const Board &b, const Move &m) {
    if (m.kind == KING) return 0;
    return piece_square_table[m.get_kind()][xfrm[b.to_move()][m.to]] -
      piece_square_table[m.get_kind()][xfrm[b.to_move()][m.from]];
  }

  ///////////////////////////////
  // Static utility functions. //
  ///////////////////////////////

  // Get the score for a piece by Kind.
  static inline Score eval_piece (Kind k) {
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

  // Get the value of a piece being captured.
  static inline Score eval_capture (const Move &m) {
    return eval_piece (m.get_capture ());
  }

  // Determine whether k is a minor piece.
  static inline bool is_minor (Kind k) {
    switch (k)
      {
      case PAWN:   return false;
      case ROOK:   return false;
      case KNIGHT: return true;
      case BISHOP: return true;
      case QUEEN:  return false;
      default:     return false;
      }
  }

  // Determine whether k is a minor piece.
  static inline bool is_major (Kind k) {
    switch (k)
      {
      case PAWN:   return false;
      case ROOK:   return true;
      case KNIGHT: return false;
      case BISHOP: return false;
      case QUEEN:  return true;
      default:     return false;
      }
  }
    
  // Get the game phase.
  inline Phase get_phase () {
    if (major_counts [WHITE] + minor_counts [WHITE] <= 3 &&
        major_counts [BLACK] + minor_counts [BLACK] <= 3)
      {
        return ENDGAME;
      }    
    else
      {
        return OPENING;
      }
  }

  //////////////////////
  // Evaluation data. //
  //////////////////////
    
  Board b;
  Phase phase;
  int piece_counts[2][5];
  int major_counts[2];
  int minor_counts[2];
  int pawn_counts[2][8];
};
  
//////////////////////////
// Piece-Square tables. //
//////////////////////////


#endif // _EVAL_
