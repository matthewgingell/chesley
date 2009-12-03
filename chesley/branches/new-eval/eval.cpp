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
#include "weights.hpp"

using namespace std;

/////////////////////
// Data structures //
/////////////////////

// The pawn evaluation cache.
PHash ph (1024 * 1024);

//////////////////////////////////////
// Evaluation function entry point. //
//////////////////////////////////////


Score Eval::sum_net_material () {
  return
    PAWN_VAL * (pop_count (b.white & b.pawns) - 
                pop_count (b.black & b.pawns)) +
    KNIGHT_VAL * (pop_count (b.white & b.knights) - 
                  pop_count (b.black & b.knights)) +
    BISHOP_VAL * (pop_count (b.white & b.bishops) - 
                  pop_count (b.black & b.bishops)) +
    ROOK_VAL * (pop_count (b.white & b.rooks) - 
                pop_count (b.black & b.rooks))  +
    QUEEN_VAL * (pop_count (b.white & b.queens) - 
                 pop_count (b.black & b.queens));
}

Score 
Eval::score () { 

  int f, f2;
  
#ifdef TRACE_EVAL
  cerr << "Evaluating " << b.to_fen () << endl;
#endif

  s += sign (b.to_move ()) * TEMPO_VAL;

  // Simple material value.
  f = b.material[WHITE] - b.material[BLACK];
  s += f;

#ifdef TRACE_EVAL
  cout << "Net material is " << f << endl;
#endif 

  // Piece square values.
  f = b.psquares[WHITE][OPENING_PHASE] -
    b.psquares[BLACK][OPENING_PHASE];

  s_op += f;

  f2 = b.psquares[WHITE][END_PHASE] - 
    b.psquares[BLACK][END_PHASE];

  s_eg += f2;

#ifdef TRACE_EVAL
  cout << "Opening psqrs is " << f << endl;
  cout << "End game psqrs is " << f2 << endl;
  cout << "Interpolated value is " << 
    interpolate (b, f, f2) << endl;
#endif   

#if 1
  // Try lazy eval
  if (s < (alpha - LAZY_EVAL_MARGIN) || (s > beta + LAZY_EVAL_MARGIN))
    return sign (b.to_move ()) * (s + interpolate (b, s_op, s_eg));
#endif

  // Compute the presence of some useful features.
  compute_features ();

  // Evaluate mobility.
  s += score_mobility (WHITE) - 
    score_mobility (BLACK);

  // King safety.
  s += score_king (WHITE) - 
    score_king (BLACK);

  // Knights.
  s += score_knight (WHITE) - 
    score_knight (BLACK);

  // Bishop.
  s += score_bishop (WHITE) - 
    score_bishop (BLACK);

  // Rooks and queens.
  s += score_rooks_and_queens (WHITE) - 
    score_rooks_and_queens (BLACK);

  // Pawn structure.

  s += score_pawns ();

  // Add interpolated value for split evaluation. 
  s += interpolate (b, s_op, s_eg);

  return sign (b.to_move ()) * s;
}

void
Eval::compute_features () {

  // Compute the set of attacked squares for white and black.
  attack_set[WHITE] = b.attack_set (WHITE);
  attack_set[BLACK] = b.attack_set (BLACK);

  // Compute the set of open files and files with only pawns of our
  // own color.
  for (File f = A; f <= H; f++)
    {
      open_file[f] = 
        (b.pawn_counts[WHITE][f] == 0 && 
         b.pawn_counts[BLACK][f]) == 0;
      half_open_file[f] =
        (b.pawn_counts[WHITE][f] == 0 || 
         b.pawn_counts[BLACK][f]) == 0;
    }
}

Score
Eval::score_king (const Color c) {
  Coord ksq = b.king_square (c);
  File  kfile = (File) idx_to_file (ksq);
  File  krank = (File) idx_to_rank (ksq);
  Score s = 0;

  // Evaluate castling status.
  if (c == WHITE) 
    {
      if (b.flags.w_has_k_castled) 
        s += CASTLED_KS_VAL;
      else if (b.flags.w_has_q_castled) 
        s += CASTLED_QS_VAL;
      else if (b.flags.w_can_k_castle)
        s += CAN_CASTLE_K_VAL;
      else if (b.flags.w_can_q_castle)
        s += CAN_CASTLE_Q_VAL;
    }
  else
    {
      if (b.flags.b_has_k_castled) 
        s += CASTLED_KS_VAL;
      else if (b.flags.b_has_q_castled) 
        s += CASTLED_QS_VAL;
      else if (b.flags.b_can_k_castle)
        s += CAN_CASTLE_K_VAL;
      else if (b.flags.b_can_q_castle)
        s += CAN_CASTLE_Q_VAL;
    }

  // Penalize kings on or next to open or half-open files.

  if (open_file[kfile]) 
    {
      s += KING_ON_OPEN_FILE_VAL;
    }
  else if (half_open_file[kfile])
    {
      s += KING_ON_HALF_OPEN_FILE_VAL;
    }

  if (kfile > A && open_file[kfile - 1]) 
    {
      s += KING_NEXT_TO_OPEN_FILE_VAL;
    }
  else if (kfile > A && half_open_file[kfile - 1])
    {
      s += KING_NEXT_TO_HALF_OPEN_FILE_VAL;
    }

  if (kfile < H && open_file[kfile + 1]) 
    {
      s += KING_NEXT_TO_OPEN_FILE_VAL;
    }
  else if (kfile < H && half_open_file[kfile + 1]) 
    {
      s += KING_NEXT_TO_OPEN_FILE_VAL;
    }

  // Evaluate pawn shield

  bitboard pawns = b.get_pawns(c);
  
  if (c == WHITE && b.has_castled (WHITE) && krank < 2)
    {
      if (test_bit (pawns, ksq << 8)) s += 10;
      if (kfile > A && test_bit (pawns, ksq << 9)) s += 10;
      if (kfile < H && test_bit (pawns, ksq << 7)) s += 10;
    }
  else if (c == BLACK && b.has_castled (BLACK)  && krank > 5)
    {
      if (test_bit (pawns, ksq << 8)) s += 10;
      if (kfile > A && c == WHITE && test_bit (pawns, ksq >> 9)) s += 10;
      if (kfile < H && c == WHITE && test_bit (pawns, ksq >> 7)) s += 10;
    }

  // Penalize the king for being adjacent to an attacked square.
  s += pop_count ((attack_set[!c] & 
                   !b.color_to_board (c) & 
                   adjacent_squares_mask (ksq))) 
    * KING_ADJACENT_ATTACKED_VAL;

  return s;
}

///////////////////////
// Evaluate knights  //
///////////////////////

Score
Eval::score_knight (const Color c) {
  Score s = 0;
 
  // Compute the set of knights protected by pawms.
  bitboard pieces = b.get_knights (c) & b.get_pawn_attacks (c);
  
  // Do very simple output evaluation on C5 / F5.
  while (pieces)
    {
      Coord idx = bit_idx (pieces);
      if (c == WHITE)
        {
          if (idx == C5 || idx == F5) s+= KNIGHT_OUTPOST_VAL;
        }
      else 
        {
          if (idx == C4 || idx == F4) s+= KNIGHT_OUTPOST_VAL;
        }
      clear_bit (pieces, idx);
    }
  
  return s;
}

//////////////////////
// Evaluate bishops //
//////////////////////

Score 
Eval::score_bishop (const Color c) {
  Score s = 0;
  bitboard our_bishops = b.get_bishops (c);
  bitboard their_pawns = b.get_pawns (!c);

  // Iterate over bishops.
  while (our_bishops) 
    {
      Coord idx = bit_idx (our_bishops);

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
  if (b.piece_counts[c][BISHOP] >= 2) s += BISHOP_PAIR_VAL;

  return s;
}

// Evaluate mobility
Score
Eval::score_mobility (const Color c) {
  Score s = 0;

  // Rooks
  bitboard pieces = b.get_rooks (c);
  while (pieces) {
    Coord idx = bit_idx (pieces);
    s += b.rook_mobility (idx) * ROOK_MOBILITY_VAL;
    clear_bit (pieces, idx);
  }

  // Knights
  pieces = b.get_knights (c);
  while (pieces) {
    Coord idx = bit_idx (pieces);
    s += b.knight_mobility (idx) * KNIGHT_MOBILITY_VAL;
    clear_bit (pieces, idx);
  }

  // Bishops
  pieces = b.get_bishops (c);
  while (pieces) {
    Coord idx = bit_idx (pieces);
    s += b.bishop_mobility (idx) * BISHOP_MOBILITY_VAL;
    clear_bit (pieces, idx);
  }

  // Queens
  pieces = b.get_queens (c);
  while (pieces) {
    Coord idx = bit_idx (pieces);
    s += b.queen_mobility (idx) * QUEEN_MOBILITY_VAL;
    clear_bit (pieces, idx);
  }
  
  return s;
}

// Evaluate rook and queen positional strength.
Score 
Eval::score_rooks_and_queens (const Color c) {
  Score s = 0;
  bitboard pieces;

  // Reward rooks on open and half open files
  pieces = b.get_rooks (c);
  while (pieces) {
    Coord idx = bit_idx (pieces);
    if (open_file [idx_to_file (idx)]) s += ROOK_OPEN_VAL;
    if (half_open_file [idx_to_file (idx)]) s += ROOK_HALF_VAL;

  // Reward rook on the 7th file trapping the enemy king.

  const int rank = idx_to_rank (idx);

  if (c == WHITE && rank == 6 && 
      (b.kings & b.black & rank_mask (7)))
    {
      s += ROOK_ON_7TH_VAL;
    }
  else if (c == BLACK && rank == 1 &&
           (b.kings & b.white & rank_mask (0)))
    {
      s += ROOK_ON_7TH_VAL;
    }

  // Rook pair on second?

    clear_bit (pieces, idx);
  }


  // Reward queens on open and half open files
  pieces = b.get_queens (c);
  while (pieces) {
    Coord idx = bit_idx (pieces);
    if (open_file [idx_to_file (idx)]) s += QUEEN_OPEN_VAL;
    if (half_open_file [idx_to_file (idx)]) s += QUEEN_HALF_VAL;
    clear_bit (pieces, idx);
  }

  return s;
}

// Memoized wrapper for score_pawns_inner.
Score
Eval::score_pawns () {
  Score s = 0;

  if (ph.lookup (b.phash, s)) 
    {
      return s;
    }
  else
    {
      s = score_pawns_inner (WHITE) - 
        score_pawns_inner (BLACK);
    }

  ph.set (b.phash, s);

  return s;
}

// Evaluate pawn structure.
Score
Eval::score_pawns_inner (const Color c) {
  Score s = 0;

  const bitboard our_pawns = b.get_pawns (c);
  const bitboard their_pawns = b.get_pawns (~c);

  bitboard i = our_pawns;
  while (i) {
    Coord idx = bit_idx (i);
    Score val = 0;

    //////////////////////////////
    // Pawn structure features. //
    //////////////////////////////

    bool backward = false;
    bool connected = false;
    bool doubled = false;
    bool isolated = false;
    bool passed = false;

    const Coord rank = idx_to_rank (idx);
    const Coord file = idx_to_file (idx);

    // A mask of the two square directly adjacent to this pawn.
    const bitboard beside_mask = 
      rank_mask (rank) & adjacent_files_mask (idx);

    // A mask of the pawns one square ahead-right and one square
    // ahead-left of this pawn.
    const bitboard front_neighbors_mask = 
      shift_forward (beside_mask, c) & our_pawns;

    // Is there an empty square in front of this pawn?
    const bool can_advance = 
      test_bit (b.unoccupied (), forward (idx, c));

    // A mask of all the squares on this and the two adjacent files
    // which are further forward than this pawn.
    const bitboard front_span = 
      (this_file_mask (idx) | adjacent_files_mask (idx)) & 
      in_front_of_mask(idx, c);

    // A mask of the squares attack by enemy pawns.
    const bitboard their_attacks = b.get_pawn_attacks (~c);

    ///////////////////
    // Doubled pawns //
    ///////////////////

    if (b.pawn_counts[c][file] > 1)
      {
        doubled = true;
      }

    //////////////////
    // Passed pawns //
    //////////////////

    // This pawn is passed if there are no enemy pawns in it's front
    // span.
    if (!(front_span & their_pawns))
      {
        passed = true;
      }

    /////////////////////
    // Backwards pawns //
    /////////////////////

    if (
        // This can advance
        can_advance 

        // And advancing it would place it besides a pawn.
        && front_neighbors_mask != 0

        // And doing so would leave it unprotected.
        && !(beside_mask & our_pawns)

        // And it could be taken by a pawn.
        && test_bit (their_attacks, forward (idx, c)))

      {
        backward = true;
      }

    //////////////////////
    // Connected pawns. //
    //////////////////////

    else if (
             
        // This pawn is connected if there is a pawn beside it.
        (beside_mask & our_pawns)
        
        // Or if it is defended by a pawn
        || (shift_backward (beside_mask, c) & our_pawns)
        
        // Or if it can advanced and place itself directly beside a pawn
        // and is not backwards.
        || (can_advance && front_neighbors_mask)

        )
      
      {
        connected = true;
      }

    /////////////////////
    // Isolated pawns. //
    /////////////////////

    // This pawn is isolated if there are no pawns of the same color
    // on the adjacent files.
    if (!(adjacent_files_mask (idx) & our_pawns))
      {
        isolated = true;
      }

    //////////////////////////////
    // Apply score adjustments. //
    //////////////////////////////
    
    if (passed && connected)
      {
        val = PASSED_CONNECTED_VAL[c][rank];
      }
    else if (passed)
      {
        val = PASSED_VAL[c][rank];
      }
    else if (connected)
      {
        val = CONNECTED_VAL[c][rank];
      }
    else if (isolated)
      {
        val = ISOLATED_VAL[c][rank];
      }

#ifdef TRACE_EVAL
    cerr << c << " pawn at " << b.to_alg_coord (idx) << ":";
    cerr << (connected ? " connected" : "");
    cerr << (doubled ? " doubled" : "");
    cerr << (isolated ? " isolated" : "");
    cerr << (passed ? " passed" : "");
    cerr << " (bonus " << val << ")" << endl;
#endif

    s += val;

    clear_bit (i, idx);
  }

#ifdef TRACE_EVAL
  cerr << endl;
#endif
  
  return s;
}
