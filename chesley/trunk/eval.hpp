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
static const Score CAN_CASTLE_VAL = 30;

///////////////////////////
// Evaluation functions. //
///////////////////////////

// Compute the net material value for this position.
inline Score eval_material (const Board &b);

// Compute a simple net positional value from the piece square table.
Score sum_piece_squares (const Board &b);

// Evaluate different kinds of piece.
inline Score eval_pawns (const Board &b, const Color c);
inline Score eval_files (const Board &b, const Color c);

// Compute a score for this position.
inline Score
eval (const Board &b) {
  Score score = 0;

  ///////////////////////
  // Evaluate material //
  ///////////////////////

#if 1
  score += eval_material (b);
#endif

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

#if 0
  score += eval_pawns (b, WHITE) - eval_pawns (b, BLACK);
  score += eval_files (b, WHITE) - eval_files (b, BLACK);
#endif

  ///////////////////////////////////
  // Evaluate positional strength. //
  ///////////////////////////////////

#if 1
  score += sum_piece_squares (b);
#endif

  ////////////////////////
  // Evaluate castling. //
  ////////////////////////

#if 1
  if (b.flags.w_can_k_castle)  score += 15;
  if (b.flags.w_can_q_castle)  score += 10;
  if (b.flags.w_has_k_castled) score += 50;
  if (b.flags.w_has_q_castled) score += 30;
  if (b.flags.b_can_k_castle)  score -= 15;
  if (b.flags.b_can_q_castle)  score -= 10;
  if (b.flags.b_has_k_castled) score -= 50;
  if (b.flags.b_has_q_castled) score -= 30;
#endif
    
  /////////////////////////////////////////////////
  // Add some random noise for variety of games. // 
  /////////////////////////////////////////////////

#if 1
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
  Score score[2];
  memset (score, 0, sizeof (score));
  int count[2][5];
  memset (count, 0, sizeof (count));

  // Count all pieces.
  for (Color c = WHITE; c <= BLACK; c++)
    for (Kind k = PAWN; k < KING; k++)   
      count [c][k] = 
	pop_count (b.color_to_board (c) & b.kind_to_board (k));

  // Sum their values.
  for (Color c = WHITE; c <= BLACK; c++)
    for (Kind k = PAWN; k < KING; k++)   
      score[c] += eval_piece (k) * count [c][k];
  
  // Provide a half-pawn bonus for having both bishops.
  for (Color c = WHITE; c <= BLACK; c++)
    if (count[c][BISHOP] >= 2) score [c] += 50;

  // Return net score.
  return score[WHITE] - score[BLACK];
}

using namespace std;

// Compute the value of the pawn structure at this position.
inline Score
eval_pawns (const Board &b, const Color c) {
  Score bonus = 0;
  bitboard pawns = b.pawns & b.color_to_board (c);
  bitboard pi = pawns;

  while (pi)
    {
      int square = bit_idx (pi);
      int file_no = b.idx_to_file (square);
      bitboard this_file = b.file_mask (file_no) & pawns;
      int file_count = pop_count (this_file);

      // Pawns on rank 0 and rank 7 can only attack one square. They
      // receive a 15% penalty.
      if (file_no == 0 || file_no == 7)
	{
	  // cerr << "penalizing rooks pawn on file " << file_no << endl;
	  bonus -= 15;
	}

#if 1
      // Penalize isolated pawns. An isolated pawn is a pawn for which
      // there is no friendly pawn of an adjacent file.
      if (file_no == 0)
	{
	  if (pop_count (b.file_mask (1) & pawns) == 0)
	    {
	      bonus -= 25;
	      // cerr << "penalizing isolated pawn on file " << file_no << endl;
	    }
	}
      else if  (file_no == 7)
	{
	  if (pop_count (b.file_mask (6) & pawns) == 0)
	    {
	      bonus -= 25;
	      // cerr << "penalizing isolated pawn on file " << file_no << endl;
	    }
	}
      else
	{
	  if (pop_count (b.file_mask (file_no - 1) & pawns) == 0 &&
	      pop_count (b.file_mask (file_no + 1) & pawns) == 0)
	    {
	      bonus -= 25;
	      // cerr << "penalizing isolated pawn on file " << file_no << endl;
	    }
	}
#endif
  
      // Penalize doubled pawns. Doubled pawns are two pawns of the same
      // color residing on the same file.
      if (file_count > 1) 
	{
	  bonus -= 25;
	  // cerr << "penalizing doubled pawn on file " << file_no << endl;
	}
      
      // Penalize backwards pawns. A backwards pawn is one that is behind
      // pawns of the same color on adjacent files which can not be
      // advanced without loss of material.

      // Reward passed pawns in the end game.

      pi = clear_lsb (pi);
    }

  // cerr << "net pawn eval = " << bonus << endl;

  return bonus;
}

inline Score 
eval_files (const Board &b, const Color c) {
  Score bonus = 0;
  bitboard col = b.color_to_board (c);
  bitboard pieces = b.rooks & b.queens & col;
  bitboard pi = pieces;

  while (pi)
    {
      int square = bit_idx (pi);
      int file_no = b.idx_to_file (square);
      if ((b.file_mask (file_no) & b.pawns & col) == 0)
	{
	  bonus += 50;
	}
      pi = clear_lsb (pi);
    }

  return bonus;
}

#endif // _EVAL_
