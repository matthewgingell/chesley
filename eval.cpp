////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// eval.cpp                                                                   //
//                                                                            //
// Copyright Matthew Gingell <matthewgingell@gmail.com>, 2009-2015. Chesley   //
// the Chess Engine! is free software distributed under the terms of the      //
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

#define PSQ 1
#define MOB 1
#define PWN 1
#define SPC 1
#define KSF 1
#define NGT 3
#define BSH 1
#define QRS 1
#define LZY 0

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

//////////////////////////////////////
// Evaluation function entry point. //
//////////////////////////////////////

Score
Eval::score () {

  Score s = 0;

  // Compute the presence of some useful features.
  compute_features ();

  // If neither side has mating material then this is a draw.
  if (can_not_win (WHITE) && can_not_win (BLACK)) return 0;

  // If we can not win but the opponent can, we still care about the
  // rest of the evaluation to guide us towards a possible draw, but
  // we apply an enormous penalty.

  else if (can_not_win (WHITE)) s -= MATE_VAL / 2;
  else if (can_not_win (BLACK)) s += MATE_VAL / 2;

  // Evaluate material.
  s += b.material[WHITE] - b.material[BLACK];

  // Piece square values.
  Score s1 = b.psquares[WHITE][OPENING_PHASE] -
    b.psquares[BLACK][OPENING_PHASE];

  Score s2 = b.psquares[WHITE][END_PHASE] -
    b.psquares[BLACK][END_PHASE];

  s += PSQ * interpolate (b, s1, s2);

#if LZY
  // Try lazy eval
  if (s < (alpha - LAZY_EVAL_MARGIN) || (s > beta + LAZY_EVAL_MARGIN))
    return s * sign (b.to_move ());
#endif

  // Mobility.
  s += MOB * (score_mobility (WHITE) - score_mobility (BLACK));

  // King safety.
  s += KSF * (score_king (WHITE) - score_king (BLACK));

  // Knights.
  s += NGT * (score_knight (WHITE) - score_knight (BLACK));

  // Bishop.
  s += BSH * (score_bishop (WHITE) - score_bishop (BLACK));

  // Rooks and queens.
  s += QRS * (score_rooks_and_queens (WHITE) -
              score_rooks_and_queens (BLACK));

  // Pawn structure.
  s += PWN * score_pawns ();

  return sign (b.to_move ()) * s;
}

void
Eval::compute_features () {

  // Compute the set of attacked squares for white and black.
  attack_set[WHITE] = b.attack_set (WHITE);
  attack_set[BLACK] = b.attack_set (BLACK);

  // Sum total material remaining on the board.
  total_material = b.material[WHITE] + b.material[BLACK];

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

  // Compute major, minor, and pawn counts.
  for (Color c = WHITE; c <= BLACK; c++)
    {
      pawn_count[c] =
        b.piece_counts[c][PAWN];

      major_count[c] =
        b.piece_counts[c][ROOK] + b.piece_counts[c][QUEEN];

      minor_count[c] = b.piece_counts[c][KNIGHT] +
        b.piece_counts[c][BISHOP];
    }
}

bool
Eval::can_not_win (Color c) {
  if (pawn_count[c] || major_count[c])
      {
        return false;
      }
    else
      {
        // Can not win with any number of bishops on the same color.
        if (b.piece_counts[c][KNIGHT] == 0)
          {
            if ((b.get_bishops(c) & light_squares) == 0 ||
                !(b.get_bishops(c) & dark_squares))
              {
                return true;
              }
          }
        else
          {
            // Can not win with just a knight.
            if (b.piece_counts[c][KNIGHT] <= 1)
              {
                return true;
              }
          }
      }

  return false;
}

Score
Eval::score_king (const Color c) {

  // CPW:
  //
  // King_Pressure: penalty for a nearby square being attack.
  // King_Attackers: number of attackers - scales king_Pressure penalty.
  // Reward for F2 etc, smaller for F3, penalty for missing.
  //
  // ideas?
  //
  // if the king could move like a queen, how many squares could it reach?
  // castle early
  // do not move the pawns in front of the king

  Score s = 0;

  Coord loc = b.king_square (c);
  int rank = idx_to_rank (loc);
  int file = idx_to_file (loc);

  /////////////////////////////
  // Pawn shield evaluation. //
  /////////////////////////////

  if (c == WHITE && rank == 0 && file >= F)
    {
      if (b.is_pawn (F2, WHITE)) s += PAWN_SHEILD_1_VAL; else
        if (b.is_pawn (F3, WHITE)) s += PAWN_SHEILD_2_VAL;
      if (b.is_pawn (G2, WHITE)) s += PAWN_SHEILD_1_VAL; else
        if (b.is_pawn (G3, WHITE)) s += PAWN_SHEILD_2_VAL;
      if (b.is_pawn (H2, WHITE)) s += PAWN_SHEILD_1_VAL; else
        if (b.is_pawn (H3, WHITE)) s += PAWN_SHEILD_2_VAL;
    }
  else if (c == WHITE && rank == 0 && file <= C)
    {
      if (b.is_pawn (A2, WHITE)) s += PAWN_SHEILD_1_VAL; else
        if (b.is_pawn (A3, WHITE)) s += PAWN_SHEILD_2_VAL;
      if (b.is_pawn (B2, WHITE)) s += PAWN_SHEILD_1_VAL; else
        if (b.is_pawn (B3, WHITE)) s += PAWN_SHEILD_2_VAL;
      if (b.is_pawn (C2, WHITE)) s += PAWN_SHEILD_1_VAL; else
        if (b.is_pawn (C3, WHITE)) s += PAWN_SHEILD_2_VAL;
    }
  else if (c == BLACK && rank == 7 && file >= F)
    {
      if (b.is_pawn (F7, BLACK)) s += PAWN_SHEILD_1_VAL; else
        if (b.is_pawn (F6, BLACK)) s += PAWN_SHEILD_2_VAL;
      if (b.is_pawn (G7, BLACK)) s += PAWN_SHEILD_1_VAL; else
        if (b.is_pawn (G6, BLACK)) s += PAWN_SHEILD_2_VAL;
      if (b.is_pawn (H7, BLACK)) s += PAWN_SHEILD_1_VAL; else
        if (b.is_pawn (H6, BLACK)) s += PAWN_SHEILD_2_VAL;
    }
  else if (c == BLACK && rank == 7 && file <= C)
    {
      if (b.is_pawn (A7, BLACK)) s += PAWN_SHEILD_1_VAL; else
        if (b.is_pawn (A6, BLACK)) s += PAWN_SHEILD_2_VAL;
      if (b.is_pawn (B7, BLACK)) s += PAWN_SHEILD_1_VAL; else
        if (b.is_pawn (B6, BLACK)) s += PAWN_SHEILD_2_VAL;
      if (b.is_pawn (C7, BLACK)) s += PAWN_SHEILD_1_VAL; else
        if (b.is_pawn (C6, BLACK)) s += PAWN_SHEILD_2_VAL;
    }

  //////////////////////
  // Reward castling. //
  //////////////////////

  if (b.has_castled (c)) s += 35;

  return s;
}

///////////////////////
// Evaluate knights  //
///////////////////////

Score
Eval::score_knight (const Color c) {
  Score s = 0;

  // Reward knights which are defended by a pawn and not attacked by a
  // pawn.

  bitboard outposts =
    b.get_knights (c) &
    b.get_pawn_attacks (c) &
    ~b.get_pawn_attacks (~c);

  int knight_outpost[64] =
    { 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 1, 4, 4, 4, 4, 1, 0,
      0, 2, 6, 8, 8, 6, 2, 0,
      0, 1, 4, 4, 4, 4, 1, 0,
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0 };

  while (outposts)
    {
      Coord idx = bit_idx (outposts);
      Coord off = (c == BLACK) ? idx : flip_white_black[idx];
      s += knight_outpost[off];
      clear_bit (outposts, idx);
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
  if (b.piece_counts[c][BISHOP] >= 2)
    s += BISHOP_PAIR_VAL;

#if 0
  // Reward bishops which are defended by a pawn and not attacked by a
  // pawn.

  bitboard outposts =
    b.get_bishops (c) &
    b.get_pawn_attacks (c) &
    ~b.get_pawn_attacks (~c);

  int bishop_outpost[64] =
    { 0, 0, 0, 0, 0, 0, 0, 0,
     -1, 0, 0, 0, 0, 0, 0,-1,
      0, 0, 1, 1, 1, 1, 0, 0,
      0, 1, 3, 3, 3, 3, 1, 0,
      0, 3, 5, 5, 5, 5, 3, 0,
      0, 1, 2, 2, 2, 2, 1, 0,
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0 };

  while (outposts)
    {
      Coord idx = bit_idx (outposts);
      Coord off = (c == BLACK) ?
        idx : flip_white_black[idx];
      s += bishop_outpost[off];
      clear_bit (outposts, idx);
    }
#endif

  return s;
}

// Evaluate mobility
Score
Eval::score_mobility (const Color c) {
  Score s = 0;
  int space = 0;
  const Coord ks = b.king_square (~c);
  bitboard our_pieces = b.color_to_board (c);
  bitboard pieces;
  bitboard attacks;

  const Score KING_GRAVITY[] = {0, 8, 7, 6, 5, 4, 3, 0};

#if 0
  // Pawns
  attacks = b.get_pawn_attacks (c) & ~our_pieces;
  s += pop_count (attacks) * PAWN_MOBILITY_VAL;
  space += pop_count (attacks & their_side_of_board (c));
#endif

  // Rooks
  pieces = b.get_rooks (c);
  while (pieces) {
    Coord idx = bit_idx (pieces);
    attacks = b.rook_attacks (idx) & ~our_pieces;
    s += pop_count (attacks) * ROOK_MOBILITY_VAL;
    space += pop_count (attacks & their_side_of_board (c));
    // s += b.rook_mobility (idx) * ROOK_MOBILITY_VAL;
    s += KING_GRAVITY[dist (idx, ks)];
    clear_bit (pieces, idx);
  }

  // Knights
  pieces = b.get_knights (c);
  while (pieces) {
    Coord idx = bit_idx (pieces);
    attacks = b.knight_attacks (idx) & ~our_pieces;
    space += pop_count (attacks & their_side_of_board (c));
    s += pop_count (attacks) * KNIGHT_MOBILITY_VAL;
    // s += b.knight_mobility (idx) * KNIGHT_MOBILITY_VAL;
    s += KING_GRAVITY[dist (idx, ks)];
    clear_bit (pieces, idx);
  }

  // Bishops
  pieces = b.get_bishops (c);
  while (pieces) {
    Coord idx = bit_idx (pieces);
    attacks = b.bishop_attacks (idx) & ~our_pieces;
    s += pop_count (attacks) * BISHOP_MOBILITY_VAL;
    space += pop_count (attacks & their_side_of_board (c));
    // s += b.bishop_mobility (idx) * BISHOP_MOBILITY_VAL;
    s += KING_GRAVITY[dist (idx, ks)];
    clear_bit (pieces, idx);
  }

  // Queens
  pieces = b.get_queens (c);
  while (pieces) {
    Coord idx = bit_idx (pieces);
    attacks = b.queen_attacks (idx) & ~our_pieces;
    space += pop_count (attacks & their_side_of_board (c));
    s += pop_count (attacks) * QUEEN_MOBILITY_VAL;
    // s += b.queen_mobility (idx) * QUEEN_MOBILITY_VAL;
    s += KING_GRAVITY[dist (idx, ks)];
    clear_bit (pieces, idx);
  }

  // Give a reward for the number of squares on the other side of the
  // board we're attacking.
  s += SPC * space;

#ifdef TRACE_EVAL
  cerr << c << " Mobility: " << s << endl;
#endif // TRACE_EVAL

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

    if (open_file [idx_to_file (idx)])
      s += ROOK_OPEN_VAL;

#if 0

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

#endif

    clear_bit (pieces, idx);
  }

#if 0
  // Reward queens on open and half open files
  pieces = b.get_queens (c);
  while (pieces) {
    Coord idx = bit_idx (pieces);
    if (open_file [idx_to_file (idx)]) s += QUEEN_OPEN_VAL;
    if (half_open_file [idx_to_file (idx)]) s += QUEEN_HALF_VAL;

    // Reward queen on the 7th file trapping the enemy king.
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
#endif

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
      s = PWN * (score_pawns_inner (WHITE) -
                 score_pawns_inner (BLACK));
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
      doubled = true;

    //////////////////
    // Passed pawns //
    //////////////////

    // This pawn is passed if there are no enemy pawns in it's front
    // span.
    if (!(front_span & their_pawns))
      passed = true;

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
        isolated = true;

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

    // Weak pawns
    if (backward || isolated || doubled)
      val -= WEAK_PAWN_VAL;

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
