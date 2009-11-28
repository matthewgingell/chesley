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

/////////////////////
// Data structures //
/////////////////////

// The pawn evaluation cache.
PHash ph (1024 * 1024);

/////////////////////////////////////////////////////////////////////////
//                                                                     //
//  piece_square_table:                                                //
//                                                                     //
//  This is a table of bonuses for each piece-location pair. The table //
//  is written in 'reverse' for readability and a transformation is    //
//  required for fetching values for black and white.                  //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

const Score piece_square_table[2][6][64] =
  {
    // Values used in the opening.
    {

      // Pawns
      {
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   5,  25,  25,   5,   0,   0,
        0,   1,  10,  25,  25,  10,   1,   0,
        0,   1,  15,  30,  30,  15,   1,   0,
        0,   1,  15,  15,  15,  15,   1,   0,
        0,   0,   0, -20, -20,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0
      },
      
      // Rooks
      {
         0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0
      },

      // Knights
      {
       -50, -20, -20, -10, -10, -20, -20, -50,
       -20,  15,  15,  25,  25,  15,  15, -20,
       -10,  15,  20,  25,  25,  20,   0, -10,
         0,  10,  20,  25,  25,  20,  10,   0,
         0,  10,  15,  20,  20,  15,  10,   0,
         0,   0,  15,  10,  10,  15,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,
       -50, -20, -20, -20, -20, -20, -20, -50
      },

      // Bishops
      {
         0,   0,   0,   5,   5,   0,   0,   0,
         0,   5,   5,   5,   5,   5,   5,   0,
         0,   5,  10,  10,  10,  10,   5,   0,
         0,   5,  10,  15,  15,  10,   5,   0,
         0,   5,  10,  15,  15,  10,   5,   0,
         0,   5,  10,  10,  10,  10,   5,   0,
         0,   5,   5,   5,   5,   5,   5,   0,
       -10, -10, -10,  -5,  -5,  10,  10, -10
      },

      // Queens
      {
         0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,  25,  50,  50,  25,   0,   0,
         0,   0,  50,  75,  75,  50,   0,   0
      },

      // Kings
      {
        -40, -40, -40, -40, -40, -40, -40, -40,
        -40, -40, -40, -40, -40, -40, -40, -40,
        -40, -40, -40, -40, -40, -40, -40, -40,
        -40, -40, -40, -40, -40, -40, -40, -40,
        -40, -40, -40, -40, -40, -40, -40, -40,
        -40, -40, -40, -40, -40, -40, -40, -40,
        -20, -20, -20, -20, -20, -20, -20, -20,
          0,  20,  40, -20,   0, -20,  40,  20
      }

    },

    // Values used in the end game.
    {
      // Pawns
      {
#define eg(x) (x + 75)
        eg( 0), eg( 0), eg( 0), eg( 0), eg( 0), eg( 0), eg( 0), eg( 0),
        eg( 0), eg( 0), eg( 5), eg( 5), eg( 5), eg( 5), eg( 0), eg( 0),
        eg( 0), eg( 1), eg( 5), eg(10), eg(10), eg( 5), eg( 1), eg( 0),
        eg( 0), eg( 1), eg(10), eg(15), eg(15), eg(10), eg( 1), eg( 0),
        eg( 0), eg( 1), eg(10), eg(10), eg(10), eg(10), eg( 1), eg( 0),
        eg( 0), eg( 0), eg( 0), eg( 0), eg( 0), eg( 0), eg( 0), eg( 0),
        eg( 0), eg( 0), eg( 0), eg( 0), eg( 0), eg( 0), eg( 0), eg( 0),
#undef eg
      },
      
      // Rooks
      {
#define eg(x) (x - 25)
        eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0)
#undef eg
      },

      // Knights
#define eg(x) (x - 25)
      {
        eg(-50), eg (-20), eg(-20), eg(-10), eg(-10), eg(-20), eg(-20), eg(-50),
        eg(-20), eg ( 15), eg( 15), eg( 25), eg( 25), eg( 15), eg( 15), eg(-20),
        eg(-10), eg ( 15), eg( 20), eg( 25), eg( 25), eg( 20), eg(  0), eg(-10),
        eg(  0), eg ( 10), eg( 20), eg( 25), eg( 25), eg( 20), eg( 10), eg(  0),
        eg(  0), eg ( 10), eg( 15), eg( 20), eg( 20), eg( 15), eg( 10), eg(  0),
        eg(  0), eg (  0), eg( 15), eg( 10), eg( 10), eg( 15), eg(  0), eg(  0),
        eg(  0), eg (  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(-50), eg (-20), eg(-20), eg(-20), eg(-20), eg(-20), eg(-20), eg(-50)
#undef eg
      },

      // Bishops
#define eg(x) (x + 25)
      {
        eg(  0), eg (  0), eg(  0), eg(  5), eg(  5), eg(  0), eg(  0), eg(  0),
        eg(  0), eg (  5), eg(  5), eg(  5), eg(  5), eg(  5), eg(  5), eg(  0),
        eg(  0), eg (  5), eg( 10), eg( 10), eg( 10), eg( 10), eg(  5), eg(  0),
        eg(  0), eg (  5), eg( 10), eg( 15), eg( 15), eg( 10), eg(  5), eg(  0),
        eg(  0), eg (  5), eg( 10), eg( 15), eg( 15), eg( 10), eg(  5), eg(  0),
        eg(  0), eg (  5), eg( 10), eg( 10), eg( 10), eg( 10), eg(  5), eg(  0),
        eg(  0), eg (  5), eg(  5), eg(  5), eg(  5), eg(  5), eg(  5), eg(  0),
        eg(-10), eg (-10), eg(-10), eg( -5), eg( -5), eg( 10), eg( 10), eg(-10)
#undef eg
      },

      // Queens
      {
#define eg(x) (x)
        eg(  0), eg (  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg (  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg (  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg (  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg (  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg (  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg (  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0),
        eg(  0), eg (  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0), eg(  0)
#undef eg
      },

      // Kings
      {
#define eg(x) (x)
        eg(  0), eg ( 10), eg( 20), eg( 30), eg( 30), eg( 20), eg( 10), eg(  0),
        eg( 10), eg ( 20), eg( 30), eg( 40), eg( 40), eg( 30), eg( 20), eg( 10),
        eg( 20), eg ( 30), eg( 40), eg( 50), eg( 50), eg( 40), eg( 30), eg( 20),
        eg( 30), eg ( 40), eg( 50), eg( 60), eg( 60), eg( 50), eg( 40), eg( 30),
        eg( 30), eg ( 40), eg( 50), eg( 60), eg( 60), eg( 50), eg( 40), eg( 30),
        eg( 20), eg ( 30), eg( 40), eg( 50), eg( 50), eg( 40), eg( 30), eg( 20),
        eg( 10), eg ( 20), eg( 30), eg( 40), eg( 40), eg( 30), eg( 20), eg( 10),
        eg(  0), eg ( 10), eg( 20), eg( 30), eg( 30), eg( 20), eg( 10), eg(  0)
#undef eg
      }
    }
  };

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

  s += sign (b.to_move ()) * TEMPO_VAL;

  // Simple material value.
  s += b.material[WHITE] - b.material[BLACK];

  // Piece square values.
  s_op += b.psquares[WHITE][OPENING_PHASE] -
    b.psquares[BLACK][OPENING_PHASE];

  s_eg += b.psquares[WHITE][END_PHASE] - 
    b.psquares[BLACK][END_PHASE];

#if 0
  // Try lazy eval
  if (s < (alpha - LAZY_EVAL_MARGIN) || (s > beta + LAZY_EVAL_MARGIN))
    return sign (b.to_move ()) * (s + interpolate (s_op, s_eg));
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
  s += interpolate (s_op, s_eg);

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
        (b.pawn_counts[WHITE][f] == 0 && b.pawn_counts[BLACK][f]) == 0;
      half_open_file[f] =
        (b.pawn_counts[WHITE][f] == 0 || b.pawn_counts[BLACK][f]) == 0;
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

// Evaluate rook and pawn positions.
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

Score
Eval::score_pawns () {
  Score s = 0;

  if (ph.lookup (b.phash, s)) 
    return s;
  else
    s = score_pawns_inner (WHITE) - score_pawns_inner (BLACK);

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

    //////////////////////////////
    // Pawn structure features. //
    //////////////////////////////

    bool passed = false;
    bool connected = false;
    bool backward = false;
    bool isolated = false;

    const Coord rank = idx_to_rank (idx);
    const Coord file = idx_to_file (idx);

    // A mask of the two square directly adjacent to this pawn.
    const bitboard beside_mask = 
      rank_mask (rank) & adjacent_files_mask (idx);

    // A mask of the pawns one square ahead-right and one square
    // ahead-left of this pawn.
    const bitboard forward_pawns_mask = 
      shift_forward (beside_mask, c) & our_pawns;

    // Is there an empty square in front of this pawn?
    const bool can_advance = test_bit (b.unoccupied (), forward (idx, c));

    // A mask of all the squares on this and the two adjacent files
    // which are further forward than this pawn.
    const bitboard front_span = 
      (this_file_mask (idx) | adjacent_files_mask (idx)) & 
      in_front_of_mask(idx, c);

    //////////////////
    // Passed pawns //
    //////////////////

#if 0
    if (idx == A4) 
      {
        print_board (this_file_mask (idx) | adjacent_files_mask (idx));
        print_board (in_front_of_mask (idx, c));
        print_board (front_span);
        print_board (their_pawns);
        print_board (front_span & their_pawns);
      }
#endif    

    // This pawn is passed if there are no enemy pawns in it's front
    // span.
    if (!(front_span & their_pawns))
      passed = true;

#if 0
    if (idx == A4) 
      {
        cout << "passed = " << passed << endl;
        exit (0);
      }
#endif

    /////////////////////
    // Backwards pawns //
    /////////////////////
    
    // If a pawn can physically move forwards and place itself
    // directly beside a pawn, but doing so would result in it's being
    // taken, the pawn is backward.

    if (can_advance && 
        forward_pawns_mask &&
        test_bit (attack_set[~c], forward (idx, c)))
      backward = true;

    //////////////////////
    // Connected pawns. //
    //////////////////////

    // This pawn is connected if there is a pawn beside it.
    if (beside_mask & our_pawns)
      connected = true;

    // Or if it is defended by a pawn
    else if (shift_backward (beside_mask, c) & our_pawns)
      connected = true;

    // Or if it can advanced and place itself directly beside a pawn
    // without being taken.
    else if (can_advance && 
             forward_pawns_mask &&
             !backward)
      connected = true;

    // We only worry about backward pawns on half-open files.
    // half_open_file (idx_to_file (idx))

    /////////////////////
    // Isolated pawns. //
    /////////////////////

    if (!(adjacent_files_mask (idx) & our_pawns))
        isolated = true;

#if 0
    cerr << c << " pawn at " << b.to_alg_coord (idx) << " is ";
    cerr << (passed ? "passed " : "");
    cerr << (backward ? "backward " : "");
    cerr << (connected ? "connected " : "");
    cerr << (isolated ? "isolated " : "");

    if (!(passed || backward || connected))
      cerr << "nothing";
    cerr << "." << endl;
#endif

    //////////////////////////////
    // Apply score adjustments. //
    //////////////////////////////

    if (backward && half_open_file[file]) 
      s += BACKWARD_VAL[c][rank];
    else if (isolated) 
      s += ISOLATED_VAL[c][rank];
    else if (connected && passed) 
      s += PASSED_CONNECTED_VAL[c][rank];
    else if (connected) 
      s += CONNECTED_VAL[c][rank];
    else if (passed) 
      s += PASSED_VAL[c][rank];
    else 
      s += BASE_ADV_VAL[c][rank];

    clear_bit (i, idx);
  }

  return s;
}

// Interpolate between the opening and end game weighted by the
// material remaining on the board.
Score 
Eval::interpolate (Score s_op, Score s_eg) {
  const int32 m_max = 2 * 
    (8 * PAWN_VAL + 2 * (ROOK_VAL + KNIGHT_VAL + BISHOP_VAL) + QUEEN_VAL);
  const int32 m = (b.material[WHITE] + b.material[BLACK]);
  return (m * s_op + (m_max - m) * s_eg) / m_max;
}
