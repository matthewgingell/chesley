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

#include <iostream>
#include "chesley.hpp"
#include "util.hpp"

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

static const Score BISHOP_PAIR_BONUS = 50;

/////////////////
// Game phase. //
/////////////////

enum Phase { OPENING, MIDGAME, ENDGAME };

inline Score eval_piece (Kind k) IS_CONST;
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

inline bool is_minor (Kind k) IS_CONST;
inline bool is_minor (Kind k) {
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

inline bool is_minor (Kind k) IS_CONST;
inline bool is_major (Kind k) {
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

// Compute a simple net positional value from the piece square table.
Score sum_piece_squares (const Board &b, Phase p) IS_CONST;

struct Eval {
  Eval (const Board &board) {
    // Initialization.
    b = board;
    memset (piece_counts, sizeof (piece_counts), 0);
    memset (minor_counts, sizeof (minor_counts), 0);
    memset (major_counts, sizeof (major_counts), 0);
    memset (pawn_counts, sizeof (pawn_counts), 0);
  }

  Score score (Score alpha IS_UNUSED = -INF, Score beta IS_UNUSED = INF) {
    Score score = 0;
    Phase phase = OPENING;

    count_material ();

    // Determine game phase.
    if (major_counts [WHITE] + minor_counts [WHITE] <= 3 &&
        major_counts [BLACK] + minor_counts [BLACK] <= 3)
      {
        phase = ENDGAME;
      }

    // If there are no pawns and no majors on the board.
    if ((piece_counts[WHITE][PAWN] == 0 && piece_counts[BLACK][PAWN] == 0)
        && (major_counts[WHITE] == 0 && major_counts[BLACK] == 0))
      {
        // If both sides have one of fewer minor pieces.
        if (minor_counts[WHITE] <= 1 && minor_counts[BLACK] <= 1)
          return 0;
      }
    
    // Generate a simple material score.
    score += score_material (WHITE) - score_material (BLACK);

    // Add net piece square values.
    score += sum_piece_squares (b, phase);

    // Provide a bonus for having castling or being able to castle.
    if (b.flags.w_has_k_castled) score += 50;
    else if (b.flags.w_has_q_castled) score += 30;
    else if (b.flags.w_can_k_castle || b.flags.w_can_q_castle) score += 10;
    if (b.flags.b_has_k_castled) score -= 50;
    else if (b.flags.w_has_q_castled) score -= 30;
    else if (b.flags.w_can_k_castle || b.flags.w_can_q_castle) score -= 10;
    
    // Provide a bonus for holding both bishops.
    if (piece_counts[WHITE][BISHOP] >= 2) score += BISHOP_PAIR_BONUS;
    if (piece_counts[BLACK][BISHOP] >= 2) score -= BISHOP_PAIR_BONUS;

    // Reward rooks and queens on open files.
    score += eval_files (WHITE) - eval_files (BLACK);

    // Evaluate bishops.
    score += eval_bishops (WHITE) - eval_bishops (BLACK);

    // Evaluate pawn structure.
    score += eval_pawns (WHITE) - eval_pawns (BLACK);

    // Evaluate board control. At the moment this seems to degrade
    // performance.
#if 0
    score += eval_control ();
#endif

    // Add a small random number for variety.
    //    score += random () % 5 - 2;
    
    // Set the appropriate sign and return the score.
    return sign (b.to_move ()) * score;
  }

  // Count the pieces on the board and populate pcounts.
  void count_material () {
    for (Color c = WHITE; c <= BLACK; c++)
      {
        // Count pieces.
        bitboard all_pieces = b.color_to_board (c);
        for (Kind k = PAWN; k < KING; k++)
          {
            int count = pop_count (all_pieces & b.kind_to_board (k));
            piece_counts[c][k] = count;
          }

        // Count pawns by file.
        for (int file = 0; file < 8; file++)
          {
            bitboard this_file = all_pieces & b.file_mask (file) & b.pawns;
            pawn_counts[c][file] = pop_count (this_file);
          }
      }

    // Count majors and minors.
    for (Color c = WHITE; c <= BLACK; c++)
      {
        major_counts[c] = piece_counts[c][ROOK] + piece_counts[c][QUEEN];
        minor_counts[c] = piece_counts[c][KNIGHT] + piece_counts[c][BISHOP];
      }
  }

  // Return a raw material for Color c.
  Score score_material (Color c) {
    Score score = 0;
    for (Kind k = PAWN; k < KING; k++)
      score += eval_piece (k) * piece_counts[c][k];
    return score;
  }

  // Reward rooks and queens on open and semi-open files.
  Score eval_files (const Color c) {
    Score score = 0;
    bitboard pieces = b.color_to_board (c) & (b.queens | b.rooks);

    while (pieces)
      {
        int idx = bit_idx (pieces);
        int file = b.idx_to_file (idx);
        int pawn_count = pawn_counts[c][file];
        if (pawn_count == 1) score += 25;
        else if (pawn_count == 0) score += 50;
        pieces = clear_lsb (pieces);
      }

    return score;
  }

  // Reward bishops not blocked by their own pawns.
  Score eval_bishops (const Color c) {
    Score score = 0;
    bitboard all = b.color_to_board (c);
    bitboard pieces = all & b.bishops;
    bitboard pawns = all & b.pawns;
    while (pieces)
      {
        // Provide a bonus when a bishop is not obstructed by pawns of
        // its own color.
        int idx = bit_idx (pieces);
        if (test_bit (Board::dark_squares, idx))
          {
            score -= pop_count (pawns & Board::dark_squares);
          }
        else
          {
            score -= pop_count (pawns & Board::light_squares);
          }
        pieces = clear_lsb (pieces);
      }

    return score * 5;
  }

  Score eval_pawns (const Color c) {
    Score score = 0;
    bitboard pawns = b.color_to_board (c) & b.pawns;


    // This appears to have no effect.
#if 0
    bitboard attacks = 0;
    if (c == WHITE)
      {
        // Capture forward right.
        attacks |= ((pawns & ~b.file_mask (0)) << 7) & b.black;
        // Capture forward left.
        attacks |= ((pawns & ~b.file_mask (7)) << 9) & b.black;
      }
    else
      {
        // Capture forward left.
        attacks |= ((pawns & ~b.file_mask (7)) >> 7) & b.white;
        // Capture forward right.
        attacks |= ((pawns & ~b.file_mask (0)) >> 9) & b.white;
      }

    score += 20 * pop_count (attacks & pawns);
#endif

    while (pawns)
      {
        int idx = bit_idx (pawns);
        int file = b.idx_to_file (idx);

        // Penalize rook pawns.
        if (file == 0 || file == 7)
          {
            score -= 15;
          }

        // Penalize doubled pawns.
        if (pawn_counts[c][file] > 1)
          {
            score -= 20;
          }

        // Penalize isolated pawns.
        if ((file == 0 && pawn_counts[c][1] == 0) ||
            (file == 7 && pawn_counts[c][6] == 0) ||
            (pawn_counts[c][file - 1] == 0 && pawn_counts[c][file + 1] == 0))
          {
            score -= 5;
          }

        pawns = clear_lsb (pawns);
      }

    return score;
  }

  // Compute the set of squares controlled by each side and adjust by
  // centrality.
  Score
  eval_control () {
    int control[64];
    memset (control, 0, sizeof (control));
    Move_Vector white_moves, black_moves;
    Score score = 0;
    Color c = b.to_move ();

    b.flags.to_move = WHITE;
    b.gen_moves (white_moves);
    for (int i = 0; i < white_moves.count; i++)
      control[white_moves[i].to] += 1 * centrality_table[i];

    b.flags.to_move = BLACK;
    b.gen_moves (black_moves);
    for (int i = 0; i < black_moves.count; i++)
      control[black_moves[i].to] -= 1 * centrality_table[i];

    for (int i = 0; i < 64; i++)
      score += control [i];

    b.flags.to_move = c;

    return score;
  }

  Board b;
  int piece_counts[2][5];
  int major_counts[2];
  int minor_counts[2];
  int pawn_counts[2][8];

  static int8 const centrality_table[64];
};

#endif // _EVAL_
