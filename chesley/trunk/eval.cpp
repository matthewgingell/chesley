////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// eval.cpp                                                                   //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include "chesley.hpp"

using namespace std;

////////////////////////////////////
// Top level evaluation function. //
////////////////////////////////////

Score 
Eval::score () {
  Score score = 0;
  phase = get_phase ();

  // Draw detection.
  if (is_draw ()) return 0;
  
  // Generate a simple material score.
  score += b.material[WHITE] - b.material[BLACK];

  // Add net piece square values.
  score += b.psquares[WHITE] - b.psquares[BLACK];

  // Evaluate pawn structure.
  score += eval_pawns (WHITE) - eval_pawns (BLACK);

  // Evaluate the king.
  score += eval_king (WHITE) - eval_king (BLACK);

  // Evaluate bishops.
  score += eval_bishops (WHITE) - eval_bishops (BLACK);

  // Reward rooks and queens on open files.
  score += eval_files (WHITE) - eval_files (BLACK);

  // Add a small random number for variety.
  score += random () % 5 - 2;
  
  // Set the appropriate sign and return the score.
  return sign (b.to_move ()) * score;
}

////////////////////////////////////////////////
// Determine whether this position is a draw. //
////////////////////////////////////////////////

// A very conservative evaluation of mating material.
bool Eval::is_draw () {
  return (piece_counts[WHITE][PAWN] == 0 && 
          piece_counts[BLACK][PAWN] == 0 &&
          major_counts[WHITE] == 0       && 
          major_counts[BLACK] == 0       &&
          minor_counts[WHITE] <= 1       &&
          minor_counts[BLACK] <= 1);
}

////////////////////////////////////////////////////////////
// Evaluate score adjustments for pieces moving on files. //
////////////////////////////////////////////////////////////

Score 
Eval::eval_files (const Color c) {
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

///////////////////////
// Evaluate bishops. //
///////////////////////

Score 
Eval::eval_bishops (const Color c) {
  Score s = 0;
  bitboard all = b.color_to_board (c);
  bitboard pieces = all & b.bishops;
  bitboard pawns = all & b.pawns;

  // Provide a bonus when a bishop is not obstructed by pawns of
  // its own color.
  while (pieces)
    {
      int idx = bit_idx (pieces);
      if (test_bit (Board::dark_squares, idx))
        {
          s -= pop_count (pawns & Board::dark_squares);
        }
      else
        {
          s -= pop_count (pawns & Board::light_squares);
        }
      pieces = clear_lsb (pieces);
    }

  // Provide a 5 point penalty for each obstructing pawn.
  s *= 5;


  // Provide a bonus for holding both bishops.
  if (piece_counts[c][BISHOP] >= 2) s += BISHOP_PAIR_BONUS;

  return s;
}
  
/////////////////////
// Evaluate pawns. //
/////////////////////

Score 
Eval::eval_pawns (const Color c) {
  Score s = 0;
  bitboard pawns = b.our_pawns (c);
  bitboard stops = b.get_pawn_moves (c);
  bitboard attacks = b.get_pawn_attacks (c);
  bitboard forward = stops & attacks;
  bitboard backward = stops & ~attacks;

  const Score backwards_penalty[8] = { 8, 12, 14, 14, 14, 14, 12, 8 };
  const Score isolated_penalty[8] = { 10, 15, 17, 17, 17, 17, 15, 10 };
  const Score doubled_penalty[8] = { 10, 15, 17, 17, 17, 17, 15, 10 };

  while (pawns)
    {
      coord idx = bit_idx (pawns);
      int file = b.idx_to_file (idx);

      // Normalize score. We would like the value of the 'average'
      // pawn to stay pretty close to PAWN_VAL.
      s += 25;
      
      // Penalize backwards pawns.
      if (b.masks_0[idx] & backward) 
        s -= backwards_penalty[file];

      // Penalize isolated pawns.
      if ((file == 0 && pawn_counts[c][1] == 0) ||
          (file == 7 && pawn_counts[c][6] == 0) ||
          (pawn_counts[c][file - 1] + pawn_counts[c][file + 1] == 0))
        s -= isolated_penalty[file];

      // Penalize doubled pawns.
      if (pawn_counts[c][file] > 1)
        s -= doubled_penalty[file];
   
      pawns = clear_lsb (pawns);
    }

  // Apply a five point bonus for every pawn who's stop square is
  // protected by a pawn.
  s += 5 * pop_count (forward);

  return s;
}

/////////////////////
// Evaluate kings. //
/////////////////////

Score 
Eval::eval_king (const Color c) {
  Score s = 0;
  coord idx = bit_idx (b.kings & b.color_to_board (c));

  // Provide a bonus for having castling or being able to castle.
  if (c == WHITE)
    {
      if (b.flags.w_has_k_castled) s += 50;
      else if (b.flags.w_can_k_castle || b.flags.w_can_q_castle) s += 10;
    }
  else
    {
      if (b.flags.b_has_k_castled) s += 50;
      else if (b.flags.b_can_k_castle || b.flags.b_can_q_castle) s += 10;
    }

  // Add the phase specific king location score.
  s += king_square_table[phase][xfrm[c][idx]];

  return s;
}


/////////////////////////////////////////////////////////
// Sum piece table values over white and black pieces. //
/////////////////////////////////////////////////////////

Score
Eval::sum_piece_squares (const Board &b) {
  Score bonus = 0;
  for (Color c = WHITE; c <= BLACK; c++)
    {
      bitboard all = b.color_to_board (c);
      int s = sign (c);

      for (Kind k = PAWN; k < KING; k++)
        {
          bitboard pieces = all & b.kind_to_board (k);
          // Do all pieces but the king.
          while (pieces)
            {
              bonus += s *
                Eval::piece_square_table[k][Eval::xfrm[c][bit_idx (pieces)]];
              pieces = clear_lsb (pieces);
            }
        }
    }
  return bonus;
}

/////////////////////////////////////////////////////////////////////////
// Initialize material count table used in the rest of the evaluation. //
/////////////////////////////////////////////////////////////////////////

void 
Eval::count_material () {
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
