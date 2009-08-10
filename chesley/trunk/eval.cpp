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

////////////////////////////////////
// Top level evaluation function. //
////////////////////////////////////

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
  s += b.material[WHITE] - b.material[BLACK];

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
#if 1
  s += rand() % 5 - 2;
#endif

#ifdef TRACE_EVAL
  cerr << "Score: " << s << endl;
#endif // TRACE_EVAL
  
  // Set the appropriate sign and return the s.
  return sign (b.to_move ()) * s;
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

Score Eval::eval_rooks (const Color c) {
  Score score = 0;
  bitboard pieces = b.get_rooks (c);
  while (pieces)
     {
       coord idx = bit_idx (pieces);
       int file = b.idx_to_file (idx);
       int rank = b.idx_to_rank (idx);
       int pawn_count = pawn_counts[c][file];

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

       pieces = clear_lsb (pieces);
     }

   return score;
}

Score Eval::eval_queens (const Color c) {
  Score s = 0;
  bitboard pieces = b.get_queens (c);
  while (pieces)
     {
       coord idx = bit_idx (pieces);
       int file = b.idx_to_file (idx);
       int pawn_count = pawn_counts[c][file];
       
       // Reward queens on open and semi-open files.
       if (pawn_count == 0) 
         { 
           s += QUEEN_OPEN_BONUS;
         }
       else if (pawn_count == 1) 
         { 
           s += QUEEN_HALF_BONUS;
         }

       pieces = clear_lsb (pieces);
     }

   return s;
}

///////////////////////
// Evaluate knights  //
///////////////////////

Score Eval::eval_knights (const Color c) {
#if 0
  Score s = 0;
  bitboard all = b.color_to_board (c);
  bitboard pieces = all & b.knights & b.get_pawn_attacks (c);

  // Reward knight outposts.
  while (pieces)
    {
      coord idx = bit_idx (pieces);
      int file = b.idx_to_file (idx);
            
      if (pawn_counts[WHITE][file] == 0 && 
          pawn_counts[BLACK][file])
        {
          s += 25;
        }

        pieces = clear_lsb (pieces);
    }

  return s;
#endif

  return 0;
}

///////////////////////
// Evaluate bishops. //
///////////////////////

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
          // Penality trapped bishops on A7 or H7.
          if ((idx == A7 && test_bit (their_pawns, B6)) ||
              (idx == H7 && test_bit (their_pawns, G6)))
            s -= BISHOP_TRAPPED_A7H7;

          // Penality trapped bishops on A6 or H6.
          if ((idx == A6 && test_bit (their_pawns, B5)) ||
              (idx == H6 && test_bit (their_pawns, G5)))
            s -= BISHOP_TRAPPED_A6H6;
        }
      else
        {
          // Penality trapped bishops on A2 or H2.
          if ((idx == A2 && test_bit (their_pawns, B3)) ||
              (idx == H2 && test_bit (their_pawns, G3)))
            s -= BISHOP_TRAPPED_A7H7;
          
          // Penality trapped bishops on A3 or H3.
          if ((idx == A3 && test_bit (their_pawns, B4)) ||
              (idx == H3 && test_bit (their_pawns, G4)))
            s -= BISHOP_TRAPPED_A6H6;
        }
      our_bishops = clear_lsb (our_bishops);
    }

  // Provide a bonus for holding both bishops.
  if (piece_counts[c][BISHOP] >= 2) s += BISHOP_PAIR_BONUS;

  return s;
}

/////////////////////////////////////
// Evaluate mobiilty by piece kind //
/////////////////////////////////////

Score 
Eval::eval_mobility (const Color c) {
  Score s = 0;
  bitboard moves;
  bitboard pieces;

  // Rooks
  pieces = b.get_rooks (c);
  while (pieces) {
    coord idx = bit_idx (pieces);
    moves = b.rook_attacks (idx);
    s += pop_count (moves) * ROOK_MOBILITY_BONUS;
    pieces = clear_lsb (pieces);
  }

  // Knights
  pieces = b.get_knights (c);
  while (pieces) {
    coord idx = bit_idx (pieces);
    moves = b.knight_attacks (idx);
    s += pop_count (moves) * KNIGHT_MOBILITY_BONUS;
    pieces = clear_lsb (pieces);
  }

  // Bishops
  pieces = b.get_bishops (c);
  while (pieces) {
    coord idx = bit_idx (pieces);
    moves = b.bishop_attacks (idx);
    s += pop_count (moves) * BISHOP_MOBILITY_BONUS;
    pieces = clear_lsb (pieces);
  }

  // Queens
  pieces = b.get_queens (c);
  while (pieces) {
    coord idx = bit_idx (pieces);
    moves = b.queen_attacks (idx);
    s += pop_count (moves) * QUEEN_MOBILITY_BONUS;
    pieces = clear_lsb (pieces);
  }

  return s;
}

/////////////////////
// Evaluate pawns. //
/////////////////////

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
    (((pawns & ~Board::file_mask (A)) << 7) | 
     ((pawns & ~Board::file_mask (H)) << 9)) :
    // Black pawns.
    (((pawns & ~Board::file_mask (H)) >> 7) |
     ((pawns & ~Board::file_mask (A)) >> 9));
}

// The union of every front span for each pawn of color c.
bitboard pawn_all_attack_spans (bitboard pawns, Color c) {
  bitboard all = 0;
  while (pawns) {
    coord idx = bit_idx (pawns);
    all |= Board::pawn_attack_spans[c][idx];
    pawns = clear_lsb (pawns);
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
  // "Originally every pawn is unfree oweing to mechanical              //
  //  obstruction. Removal of its counter pawn makes a pawn half-free." //
  //                                                                    //
  ////////////////////////////////////////////////////////////////////////
  
  bitboard half_free = our_pawns;
  for (int i = 0; i < 8; i++)
    if (pawn_counts[!c][i] > 0) 
      half_free &= ~Board::file_mask (i);

  //////////////////////////////////////////////////////////////////////
  //                                                                  //
  //  Compute the set of backward pawns.                              //
  //                                                                  //
  // "A half-free pawn, placed on the second or third rank, whose     //
  //  stopsquare lacks pawn protection but is controlled by a sentry, //
  //  is called a backwards pawn."                                    //
  //                                                                  //
  //////////////////////////////////////////////////////////////////////

  bitboard backwards;

  // Half-free pawns on the second or third rank.
  backwards = (c == WHITE) ?
    (half_free & (Board::rank_mask (1) | Board::rank_mask (2))) :
    (half_free & (Board::rank_mask (6) | Board::rank_mask (5)));

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
  
  ////////////////////////////////////////////////////////////////////
  // Iterate over the set of pawns assessing penalties and bonuses. //
  ////////////////////////////////////////////////////////////////////

  bitboard p = our_pawns;
  while (p)
    {
      coord idx = bit_idx (p);
      int rank = b.idx_to_rank (idx);
      int file = b.idx_to_file (idx);
      
      // Penalize isolated pawns.
      if (!(Board::adjacent_files_mask(idx) & our_pawns))
        s -= isolated_penalty[file];

      // Penalize backwards pawns.
      if (test_bit (backwards, idx)) 
        s -= backwards_penalty[file];

      // Penalize doubled pawns.
      if (pawn_counts[c][file] > 1) 
        s -= doubled_penalty[file];

      // Reward passed pawns.
      bitboard front = 
        (Board::this_file_mask (idx) | 
         Board::adjacent_files_mask (idx)) 
        & Board::in_front_of_mask(idx, c);
      if (!(front & their_pawns))
        s += passed_pawn_bonus[c][rank];

      // Further reward passed pawn backed by rook?

      p = clear_lsb (p);
    }

  // Save this score in the pawn eval cache.
  ph.set (b.phash, s);

  return s;
}

/////////////////////
// Evaluate kings. //
/////////////////////

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

  return s;
}


/////////////////////////////////////////////////////////
// Sum piece table values over white and black pieces. //
/////////////////////////////////////////////////////////

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

void Eval::count_material () {
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
