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
#include "board.hpp"
#include "chesley.hpp"
#include "phash.hpp"

using namespace std;

// The pawn evaluation cache.
PHash ph (1024 * 1024);

///////////////////////////////////
// Top level evaluation function //
///////////////////////////////////

Score Eval::score () {

#ifdef TRACE_EVAL
  cerr << "Evaluating position " << b.to_fen () << endl;
  cerr << b << endl << endl;
#endif // TRACE_EVAL

  Score s = sign (b.to_move ()) * TEMPO_BONUS;
  phase = get_phase ();

  // Draw detection.
  if (is_draw ()) return 0;
  
  // Generate a simple material score.
  s += b.net_material;

  // Add net piece square values.
  s += (b.psquares[WHITE] - b.psquares[BLACK]);

  // Mobility
  s += eval_mobility (WHITE) - eval_mobility (BLACK);

  // Evaluate by kind of piece.
  s += eval_pawns   (WHITE) - eval_pawns   (BLACK);
  s += eval_rooks   (WHITE) - eval_rooks   (BLACK);
  s += eval_knights (WHITE) - eval_knights (BLACK);
  s += eval_bishops (WHITE) - eval_bishops (BLACK);
  s += eval_queens  (WHITE) - eval_queens  (BLACK);
  s += eval_king    (WHITE) - eval_king    (BLACK);
  
  // Add a small random number for variety.
#if 0
  s += rand() % 5 - 2;
#endif

#ifdef TRACE_EVAL
  cerr << "Score: " << s << endl;
#endif // TRACE_EVAL
  
  // Set the appropriate sign and return the s.
  return sign (b.to_move ()) * s;
}

///////////////////////////////////////////////
// Determine whether this position is a draw //
///////////////////////////////////////////////

// A very conservative evaluation of mating material.
bool Eval::is_draw () {
  return (b.piece_counts[WHITE][PAWN] == 0 && 
          b.piece_counts[BLACK][PAWN] == 0 &&
          major_counts[WHITE] == 0       && 
          major_counts[BLACK] == 0       &&
          minor_counts[WHITE] <= 1       &&
          minor_counts[BLACK] <= 1);
}

///////////////////////////////////////////////////////////
// Evaluate score adjustments for pieces moving on files //
///////////////////////////////////////////////////////////

Score Eval::eval_rooks (const Color c) {
  Score score = 0;
  bitboard pieces = b.get_rooks (c);
  while (pieces)
     {
       coord idx = bit_idx (pieces);
       int file = idx_to_file (idx);
       int rank = idx_to_rank (idx);
       int pawn_count = b.pawn_counts[c][file];

       // Reward rooks on open and semi-open files.
       if (pawn_count == 0) 
         { 
           score += ROOK_OPEN_BONUS;
         }
       else if (pawn_count == 1) 
         { 
           score += ROOK_HALF_BONUS;
         }

       // Reward rook on the 7th file.
       if (c == WHITE && rank == 6)
         {
           score += ROOK_ON_7TH_BONUS;
         }
       else if (c == BLACK && rank == 1)
         {
           score += ROOK_ON_7TH_BONUS;
         }

       clear_bit (pieces, idx);
     }

   return score;
}

Score Eval::eval_queens (const Color c) {
  Score s = 0;
  bitboard pieces = b.get_queens (c);
  while (pieces)
     {
       coord idx = bit_idx (pieces);
       int file = idx_to_file (idx);
       int pawn_count = b.pawn_counts[c][file];
       
       // Reward queens on open and semi-open files.
       if (pawn_count == 0) 
         { 
           s += QUEEN_OPEN_BONUS;
         }
       else if (pawn_count == 1) 
         { 
           s += QUEEN_HALF_BONUS;
         }

       clear_bit (pieces, idx);
     }

   return s;
}

///////////////////////
// Evaluate knights  //
///////////////////////

Score Eval::eval_knights (const Color c) {
  Score s = 0;
 
  // Compute the set of knights protected by pawms.
  bitboard pieces = b.get_knights (c) & b.get_pawn_attacks (c);
  
  // Do very simple output evaluation on C5 / F5.
  while (pieces)
    {
      coord idx = bit_idx (pieces);
      if (c == WHITE)
        {
          if (idx == C5 || idx == F5) s+= KNIGHT_OUTPOST_BONUS;
        }
      else 
        {
          if (idx == C4 || idx == F4) s+= KNIGHT_OUTPOST_BONUS;
        }
      clear_bit (pieces, idx);
    }
  
  return s;
}

//////////////////////
// Evaluate bishops //
//////////////////////

Score Eval::eval_bishops (const Color c) {
  Score s = 0;
  bitboard our_bishops = b.get_bishops (c);
  bitboard their_pawns = b.get_pawns (!c);

  // Iterate over bishops.
  while (our_bishops) 
    {
      coord idx = bit_idx (our_bishops);

      if (c == WHITE)
        {
          // Penalty trapped bishops on A7 or H7.
          if ((idx == A7 && test_bit (their_pawns, B6)) ||
              (idx == H7 && test_bit (their_pawns, G6)))
            s -= BISHOP_TRAPPED_A7H7;

          // Penalty trapped bishops on A6 or H6.
          if ((idx == A6 && test_bit (their_pawns, B5)) ||
              (idx == H6 && test_bit (their_pawns, G5)))
            s -= BISHOP_TRAPPED_A6H6;
        }
      else
        {
          // Penalty trapped bishops on A2 or H2.
          if ((idx == A2 && test_bit (their_pawns, B3)) ||
              (idx == H2 && test_bit (their_pawns, G3)))
            s -= BISHOP_TRAPPED_A7H7;
          
          // Penalty trapped bishops on A3 or H3.
          if ((idx == A3 && test_bit (their_pawns, B4)) ||
              (idx == H3 && test_bit (their_pawns, G4)))
            s -= BISHOP_TRAPPED_A6H6;
        }
      clear_bit (our_bishops, idx);
    }

  // Provide a bonus for holding both bishops.
  if (b.piece_counts[c][BISHOP] >= 2) s += BISHOP_PAIR_BONUS;

  return s;
}

/////////////////////////////////////
// Evaluate mobility by piece kind //
/////////////////////////////////////

Score 
Eval::eval_mobility (const Color c) {
  Score s = 0;
  bitboard pieces;

  // Rooks
  pieces = b.get_rooks (c);
  while (pieces) {
    coord idx = bit_idx (pieces);
    s += b.rook_mobility (idx) * ROOK_MOBILITY_BONUS;
    clear_bit (pieces, idx);
  }

  // Knights
  pieces = b.get_knights (c);
  while (pieces) {
    coord idx = bit_idx (pieces);
    s += b.knight_mobility (idx) * KNIGHT_MOBILITY_BONUS;
    clear_bit (pieces, idx);
  }

  // Bishops
  pieces = b.get_bishops (c);
  while (pieces) {
    coord idx = bit_idx (pieces);
    s += b.bishop_mobility (idx) * BISHOP_MOBILITY_BONUS;
    clear_bit (pieces, idx);
  }

  // Queens
  pieces = b.get_queens (c);
  while (pieces) {
    coord idx = bit_idx (pieces);
    s += b.queen_mobility (idx) * QUEEN_MOBILITY_BONUS;
    clear_bit (pieces, idx);
  }

  return s;
}

////////////////////
// Evaluate pawns //
////////////////////

// Return the set of squares directly in front of a set of pawns.
bitboard pawn_forward_rank (bitboard pawns, Color c) {
  return (c == WHITE) ? (pawns << 8) : (pawns >> 8);
}

// Return the set of squares directly behind of a set of pawns.
bitboard pawn_backward_rank (bitboard pawns, Color c) {
  return (c == WHITE) ? (pawns >> 8) : (pawns << 8);
}

// Return the set of squares attacked by a set of pawns, excluding the
// En Passant square.
bitboard pawn_attacks (bitboard pawns, Color c) {
  return (c == WHITE) ? 
    // White pawns.
    (((pawns & ~file_mask (A)) << 7) | 
     ((pawns & ~file_mask (H)) << 9)) :
    // Black pawns.
    (((pawns & ~file_mask (H)) >> 7) |
     ((pawns & ~file_mask (A)) >> 9));
}

// The union of every front span for each pawn of color c.
bitboard pawn_all_attack_spans (bitboard pawns, Color c) {
  bitboard all = 0;
  while (pawns) {
    coord idx = bit_idx (pawns);
    all |= pawn_attack_spans[c][idx];
    clear_bit (pawns, idx);
  }
  return all;
}

Score Eval::eval_pawns (const Color c) {
  // All the following block quotes regarding pawn structure are taken
  // from "Pawn Power in Chess" by Hans Kmoch.
  
  Score s;
  const bitboard our_pawns = b.get_pawns (c);
  bitboard their_pawns = b.get_pawns (!c);

  // Try to find this entry in the cache.
  if (ph.lookup (b.phash, s))
    {
      return s;
    }
  else
    {
      s = 0;
    }

  ////////////////////////////////////////////////////////////////////////
  //                                                                    //
  // Compute the set of half-free pawns.                                //
  //                                                                    //
  // "Originally every pawn is unfree owing to mechanical               //
  //  obstruction. Removal of its counter pawn makes a pawn             //
  //  half-free."                                                       //
  //                                                                    //
  ////////////////////////////////////////////////////////////////////////
  
  bitboard half_free = our_pawns;
  for (int i = 0; i < 8; i++)
    if (b.pawn_counts[!c][i] > 0) 
      half_free &= ~file_mask (i);

  //////////////////////////////////////////////////////////////////////
  //                                                                  //
  //  Compute the set of backward pawns.                              //
  //                                                                  //
  // "A half-free pawn, placed on the second or third rank, whose     //
  //  stop square lacks pawn protection but is controlled by a        //
  //  sentry, is called a backwards pawn."                            //
  //                                                                  //
  //////////////////////////////////////////////////////////////////////

  bitboard backwards;

  // Half-free pawns on the second or third rank.
  backwards = (c == WHITE) ?
    (half_free & (rank_mask (1) | rank_mask (2))) :
    (half_free & (rank_mask (6) | rank_mask (5)));

  // Lacking protection on the stop square.
  backwards = pawn_forward_rank (backwards, c);
  backwards &= ~pawn_attacks (our_pawns, c);

  // Threatened by a sentry.
  backwards &= pawn_all_attack_spans (their_pawns, !c);
  backwards = pawn_backward_rank (backwards, c);

#ifdef TRACE_EVAL
  cerr << "Backwards pawns for: " << c << endl;
  print_board (backwards);
#endif // TRACE_EVAL
  
  ///////////////////////////////////////////////////////////////////
  // Iterate over the set of pawns assessing penalties and bonuses //
  ///////////////////////////////////////////////////////////////////

  bitboard p = our_pawns;
  while (p)
    {
      coord idx = bit_idx (p);
      int rank = idx_to_rank (idx);
      int file = idx_to_file (idx);
      
      // Penalize isolated pawns.
      if (!(adjacent_files_mask(idx) & our_pawns))
        s -= isolated_penalty[file];

      // Penalize backwards pawns.
      if (test_bit (backwards, idx)) 
        s -= backwards_penalty[file];

      // Penalize doubled pawns.
      if (b.pawn_counts[c][file] > 1) 
        s -= doubled_penalty[file];

      // Reward passed pawns.
      bitboard front = 
        (this_file_mask (idx) | 
         adjacent_files_mask (idx))
        & in_front_of_mask(idx, c);
      if (!(front & their_pawns))
        s += passed_pawn_bonus[c][rank];

      // Further reward passed pawn backed by rook?

      clear_bit (p, idx);
    }

  // Save this score in the pawn eval cache.
  ph.set (b.phash, s);

  return s;
}

////////////////////
// Evaluate kings //
////////////////////

Score Eval::eval_king (const Color c) {
  Score s = 0;

  // Handle the case of no king in a test position.
  if ((b.kings & b.color_to_board (c)) == 0)
    return 0;

  coord idx = bit_idx (b.kings & b.color_to_board (c));

  // Provide a bonus for having castled or being able to castle.
  if (c == WHITE)
    {
      if (b.flags.w_has_k_castled) 
        {
          s += CASTLE_KS_BONUS;
        }
      else if (b.flags.w_has_q_castled) 
        {
          s += CASTLE_QS_BONUS;
        }
      else if (b.flags.w_can_k_castle || b.flags.w_can_q_castle)
        {
          s+= CAN_CASTLE_BONUS;
        }
    }
  else
    {
      if (b.flags.b_has_k_castled) 
        {
          s += CASTLE_KS_BONUS;
        }
      else if (b.flags.b_has_q_castled) 
        {
          s += CASTLE_QS_BONUS;
        }
      else if (b.flags.b_can_k_castle || b.flags.b_can_q_castle)
        {
          s += CAN_CASTLE_BONUS;
        }
    }
  
  // Add the phase specific king location score.
  s += king_square_table[phase][xfrm[c][idx]];

  // Very basic pawn shield evaluation.
  if (phase < ENDGAME)
    {
      int rank = idx_to_rank (idx);
      int file = idx_to_file (idx);
      bitboard pawns = b.get_pawns (c);

      if (c == WHITE && 
          (b.flags.w_has_k_castled || b.flags.w_has_q_castled) &&
          rank == 0) 
        {
          for (int f = max (file - 1, 0); f <= min (file + 1, 7); f++)
            {
              if (test_bit (pawns, to_idx (rank + 1, f))) 
                s += PAWN_SHEILD1_BONUS;
              else if (test_bit (pawns, to_idx (rank + 2, f))) 
                s += PAWN_SHEILD2_BONUS;
            }
        }
      else if (c == BLACK && 
               (b.flags.b_has_k_castled || b.flags.b_has_q_castled) &&
               rank == 7) 
        {
          for (int f = max (file - 1, 0); f <= min (file + 1, 7); f++)
            {
              if (test_bit (pawns, to_idx (rank - 1, f))) 
                s += PAWN_SHEILD1_BONUS;
              else if (test_bit (pawns, to_idx (rank - 2, f))) 
                s += PAWN_SHEILD2_BONUS;
            }
        }
    }

  return s;
}

////////////////////////////////////////////////////////
// Sum piece table values over white and black pieces //
////////////////////////////////////////////////////////

Score Eval::sum_piece_squares (const Board &b) {
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
              coord idx = bit_idx (pieces);
              bonus += s *
                Eval::piece_square_table[k][Eval::xfrm[c][idx]];
              clear_bit (pieces, idx);
            }
        }
    }
  return bonus;
}

////////////////////////////////////////////////////////////////////////
// Initialize material count table used in the rest of the evaluation //
////////////////////////////////////////////////////////////////////////

void Eval::count_material () {
  // Count majors and minors.
  for (Color c = WHITE; c <= BLACK; c++)
    {
      major_counts[c] = b.piece_counts[c][ROOK] + b.piece_counts[c][QUEEN];
      minor_counts[c] = b.piece_counts[c][KNIGHT] + b.piece_counts[c][BISHOP];
    }
}
