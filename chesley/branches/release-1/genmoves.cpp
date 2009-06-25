////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// genmoves.cpp                                                               //
//                                                                            //
// Code to generate a vector of moves, given a state of play.                 //
//                                                                            //
// There is a good discussion of generating moves from bitmaps in:            //
// _Rotated bitmaps, a new twist on an old idea_ by Robert Hyatt at           //
// http://www.cis.uab.edu/info/faculty/hyatt/bitmaps.html.                    //
//                                                                            //
// The code here is very sensitive and changes should be checked              //
// against the perft suite to ensure they do not cause regressions.           //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <iostream>

#include "chesley.hpp"

using namespace std;

//////////////////////////////////
// Attack table look up macros. //
//////////////////////////////////

#define RANK_ATTACKS(idx) \
  (RANK_ATTACKS_TBL     [idx * 256 + occ_0 (idx)])

#define FILE_ATTACKS(idx) \
  (FILE_ATTACKS_TBL     [idx * 256 + occ_90 (idx)])

#define DIAG_45_ATTACKS(idx) \
  (DIAG_45_ATTACKS_TBL  [idx * 256 + occ_45 (idx)])

#define DIAG_135_ATTACKS(idx) \
  (DIAG_135_ATTACKS_TBL [idx * 256 + occ_135 (idx)])

#define KNIGHT_ATTACKS(idx) (KNIGHT_ATTACKS_TBL[idx])
#define BISHOP_ATTACKS(idx) (DIAG_45_ATTACKS(idx) | DIAG_135_ATTACKS(idx))
#define ROOK_ATTACKS(idx)   (RANK_ATTACKS(idx) | FILE_ATTACKS(idx))
#define QUEEN_ATTACKS(idx)  (BISHOP_ATTACKS(idx) | ROOK_ATTACKS(idx))
#define KING_ATTACKS(idx)   (KING_ATTACKS_TBL[idx])

// Collect all possible moves.
void
Board::gen_moves (Move_Vector &moves) const
{
  Color c = to_move ();

  ////////////
  // Pawns. //
  ////////////

  bitboard our_pawns = pawns & our_pieces ();

  // For each pawn:
  while (our_pawns)
    {
      bitboard from = clear_msbs (our_pawns);
      bitboard to;

      if (c == WHITE)
        {
          // Empty square on step forwards.
          to = (from << 8) & unoccupied ();

          // Pawns coming from rank 1 with empty square one and two steps
          // forwards.
          to |= ((to & rank_mask (2)) << 8) & unoccupied ();

          // Capture forward right.
          to |= ((from & ~file_mask (A)) << 7) & black;

          // Capture forward left.
          to |= ((from & ~file_mask (H)) << 9) & black;

          // Pawns which can reach the En Passant square.
          if (flags.en_passant != 0 &&
              ((bit_idx ((from & ~file_mask (A)) << 7) == flags.en_passant) ||
               (bit_idx ((from & ~file_mask (H)) << 9) == flags.en_passant)))
            {
              to |= masks_0[flags.en_passant];
            }
        }
      else
        {
          // Empty square on step forwards.
          to = (from >> 8) & unoccupied ();

          // Pawns coming from rank 6 with empty square one and two steps
          // forwards.
          to |= ((to & rank_mask (5)) >> 8) & unoccupied ();

          // Capture forward left.
          to |= ((from & ~file_mask (H)) >> 7) & white;

          // Capture forward right.
          to |= ((from & ~file_mask (A)) >> 9) & white;

          // Pawns which can reach the En Passant square.
          if (flags.en_passant != 0 &&
              ((bit_idx ((from & ~file_mask (H)) >> 7) == flags.en_passant) ||
               (bit_idx ((from & ~file_mask (A)) >> 9) == flags.en_passant)))
            {
              to |= masks_0[flags.en_passant];
            }
        }

      // Collect each destination in the moves list.
      int from_idx = bit_idx (from);
      while (to)
        {
          int to_idx = bit_idx (to);

          // Handle the case of a promotion.
          if ((c == WHITE && idx_to_rank (to_idx) == 7) ||
              (c == BLACK && idx_to_rank (to_idx) == 0))
            {
              // Generate a move for each possible promotion.
              for (Kind k = ROOK; k <= QUEEN; k++)
                {
                  moves.push (from_idx, to_idx, k);
                }
            }
          else
            {
              // Handle non-promotion case;
              moves.push (from_idx, to_idx);
            }

          to = clear_lsb (to);
        }

      our_pawns = clear_lsb (our_pawns);
    }

  ////////////
  // Rooks. //
  ////////////

  bitboard our_rooks = rooks & our_pieces ();

  // For each rook
  while (our_rooks)
    {
      int from = bit_idx (our_rooks);

      // Collect each destination in the moves list.
      bitboard to = ROOK_ATTACKS(from) & ~our_pieces ();
      while (to)
        {
          int to_idx = bit_idx (to);
          moves.push (from, to_idx);
          to = clear_lsb (to);
        }

      our_rooks = clear_lsb (our_rooks);
    }

  //////////////
  // Knights. //
  //////////////

  bitboard our_knights = knights & our_pieces ();

  // For each knight:
  while (our_knights)
    {
      int from = bit_idx (our_knights);

      // Collect each destination in the moves list.
      bitboard to = KNIGHT_ATTACKS(from) & ~our_pieces ();
      while (to)
        {
          int to_idx = bit_idx (to);
          moves.push (from, to_idx);
          to = clear_lsb (to);
        }
      our_knights = clear_lsb (our_knights);
    }

  //////////////
  // Bishops. //
  //////////////

  bitboard our_bishops = bishops & our_pieces ();

  // For each bishop;
  while (our_bishops)
    {
      int from = bit_idx (our_bishops);

      // Collect each destination in the moves list.
      bitboard to = BISHOP_ATTACKS (from) & ~our_pieces ();
      while (to)
        {
          int to_idx = bit_idx (to);
          moves.push (from, to_idx);
          to = clear_lsb (to);
        }
      our_bishops = clear_lsb (our_bishops);
    }

  /////////////
  // Queens. //
  /////////////

  bitboard our_queens = queens & our_pieces ();

  // For each queen.
  while (our_queens)
    {
      int from = bit_idx (our_queens);

      // Collect each destination in the moves list.
      bitboard to = QUEEN_ATTACKS (from) & ~our_pieces ();
      while (to)
        {
          int to_idx = bit_idx (to);
          moves.push (from, to_idx);
          to = clear_lsb (to);
        }
      our_queens = clear_lsb (our_queens);
    }

  ////////////
  // Kings. //
  ////////////

  bitboard our_king = kings & our_pieces ();

  ///////////////////////////
  // Compute simple moves. //
  ///////////////////////////

  if (our_king)
    {
      int from = bit_idx (our_king);

      // Collect each destination in the moves list.
      bitboard to = KING_ATTACKS (from) & ~our_pieces ();
      while (to)
        {
          int to_idx = bit_idx (to);
          moves.push (from, to_idx);
          to = clear_lsb (to);
        }
    }

  /////////////////////////////
  // Compute castling moves. //
  /////////////////////////////

  if (c == WHITE)
    {
      byte row = occ_0 (E1);

      if (flags.w_can_q_castle && !(row & 0xE))
        {
          moves.push (E1, C1);
        }

      if (flags.w_can_k_castle && !(row & 0x60))
        {
          moves.push (E1, G1);
        }
    }
  else
    {
      byte row = occ_0 (E8);

      if (flags.b_can_q_castle && !(row & 0xE))
        {
          moves.push (E8, C8);
        }

      if (flags.b_can_k_castle && !(row & 0x60))
        {
          moves.push (E8, G8);
        }
    }
}

// Generate captures
void
Board::gen_captures (Move_Vector &moves) const
{
  Color c = to_move ();

  /////////////////////////////
  // Generate pawn captures. //
  /////////////////////////////

  bitboard our_pawns = pawns & our_pieces ();
  while (our_pawns)
    {
      bitboard from = clear_msbs (our_pawns);
      bitboard to = 0;

      if (c == WHITE)
        {
          // Capture forward right.
          to |= ((from & ~file_mask (A)) << 7) & black;

          // Capture forward left.
          to |= ((from & ~file_mask (H)) << 9) & black;

          // Pawns which can reach the En Passant square.
          if (flags.en_passant != 0 &&
              ((bit_idx ((from & ~file_mask (A)) << 7) == flags.en_passant) ||
               (bit_idx ((from & ~file_mask (H)) << 9) == flags.en_passant)))
            {
              to |= masks_0[flags.en_passant];
            }
        }
      else
        {
          // Capture forward left.
          to |= ((from & ~file_mask (H)) >> 7) & white;

          // Capture forward right.
          to |= ((from & ~file_mask (A)) >> 9) & white;

          // Pawns which can reach the En Passant square.
          if (flags.en_passant != 0 &&
              ((bit_idx ((from & ~file_mask (H)) >> 7) == flags.en_passant) ||
               (bit_idx ((from & ~file_mask (A)) >> 9) == flags.en_passant)))
            {
              to |= masks_0[flags.en_passant];
            }
        }

      // Collect each destination in the moves list.
      int from_idx = bit_idx (from);
      while (to)
        {
          int to_idx = bit_idx (to);
          moves.push (from_idx, to_idx);
          to = clear_lsb (to);
        }
      our_pawns = clear_lsb (our_pawns);
    }

  /////////////////////////////
  // Generate rook captures. //
  /////////////////////////////

  bitboard our_rooks = rooks & our_pieces ();

  // For each rook
  while (our_rooks)
    {
      int from = bit_idx (our_rooks);

      // Collect each destination in the moves list.
      bitboard to = ROOK_ATTACKS (from) & ~our_pieces ();
      to &= other_pieces ();
      while (to)
        {
          int to_idx = bit_idx (to);
          moves.push (from, to_idx);
          to = clear_lsb (to);
        }

      our_rooks = clear_lsb (our_rooks);
    }

  ///////////////////////////////
  // Generate knight captures. //
  ///////////////////////////////

  bitboard our_knights = knights & our_pieces ();

  // For each knight:
  while (our_knights)
    {
      int from = bit_idx (our_knights);

      // Collect each destination in the moves list.
      bitboard to = KNIGHT_ATTACKS (from) & ~our_pieces ();
      to &= other_pieces ();
      while (to)
        {
          int to_idx = bit_idx (to);
          moves.push (from, to_idx);
          to = clear_lsb (to);
        }
      our_knights = clear_lsb (our_knights);
    }

  ///////////////////////////////
  // Generate bishop captures. //
  ///////////////////////////////

  bitboard our_bishops = bishops & our_pieces ();

  // For each bishop;
  while (our_bishops)
    {
      // Look up destinations in moves table and collect each in 'moves'.
      int from = bit_idx (our_bishops);

      // Collect each destination in the moves list.
      bitboard to = BISHOP_ATTACKS (from) & ~our_pieces ();
      to &= other_pieces ();
      while (to)
        {
          int to_idx = bit_idx (to);
          moves.push (from, to_idx);
          to = clear_lsb (to);
        }
      our_bishops = clear_lsb (our_bishops);
    }

  //////////////////////////////
  // Generate queen captures. //
  //////////////////////////////

  bitboard our_queens = queens & our_pieces ();

  // For each queen.
  while (our_queens)
    {
      // Look up destinations in moves table and collect each in 'moves'.
      int from = bit_idx (our_queens);

      // Collect each destination in the moves list.
      bitboard to = QUEEN_ATTACKS (from);
      to &= other_pieces ();
      while (to)
        {
          int to_idx = bit_idx (to);
          moves.push (from, to_idx);
          to = clear_lsb (to);
        }
      our_queens = clear_lsb (our_queens);
    }

  /////////////////////////////
  // Generate king captures. //
  /////////////////////////////

  bitboard our_king = kings & our_pieces ();

  if (our_king)
    {
      int from = bit_idx (our_king);

      // Collect each destination in the moves list.
      bitboard to = KING_ATTACKS (from) & ~our_pieces ();
      to &= other_pieces ();
      while (to)
        {
          int to_idx = bit_idx (to);
          moves.push (from, to_idx);
          to = clear_lsb (to);
        }
    }
}

// Compute a bitboard of every square color is attacking.
bitboard
Board::attack_set (Color c) const {
  bitboard color = c == WHITE ? white : black;
  bitboard attacks = 0llu;
  bitboard pieces;
  int from;

  // Pawns
  pieces = pawns & color;
  if (c == WHITE)
    {
      attacks |= ((pieces & ~file_mask (A)) << 7);
      attacks |= ((pieces & ~file_mask (H)) << 9);
    }
  else
    {
      attacks |= ((pieces & ~file_mask (A)) >> 9);
      attacks |= ((pieces & ~file_mask (H)) >> 7);
    }

  // Rooks
  pieces = rooks & color;
  while (pieces)
    {
      from = bit_idx (pieces);
      attacks |= ROOK_ATTACKS (from);
      pieces = clear_lsb (pieces);
    }

  // Knights
  pieces = knights & color;
  while (pieces)
    {
      from = bit_idx (pieces);
      attacks |= KNIGHT_ATTACKS (from);
      pieces = clear_lsb (pieces);
    }

  // Bishops
  pieces = bishops & color;
  while (pieces)
    {
      from = bit_idx (pieces);
      attacks |= BISHOP_ATTACKS (from);
      pieces = clear_lsb (pieces);
    }

  // Queens
  pieces = queens & color;
  while (pieces)
    {
      from = bit_idx (pieces);
      attacks |= QUEEN_ATTACKS (from);
      pieces = clear_lsb (pieces);
    }

  // Kings
  pieces = kings & color;
  while (pieces)
    {
      from = bit_idx (pieces);
      attacks |= KING_ATTACKS (from);
      pieces = clear_lsb (pieces);
    }

  return attacks & ~color;
}

// Get the number of legal moves available from this position.
int
Board::child_count () const
{
  int count = 0;
  Move_Vector moves (*this);

  for (int i = 0; i < moves.count; i++)
    {
      Board c = *this;
      if (c.apply (moves[i])) count++;
    }

  return count;
}

// Return whether the square at idx is attacked by a piece of color c.
bool
Board::is_attacked (int idx, Color c) const
{
  // Take advantage of the symmetry that if the piece at index could
  // move like an X and capture an X, then that X is able to attack it

  bitboard them = color_to_board (c);

  // Are we attack by a non-pawn?
  if ((ROOK_ATTACKS   (idx) & them & (queens | rooks))   ||
      (BISHOP_ATTACKS (idx) & them & (queens | bishops)) ||
      (KNIGHT_ATTACKS (idx) & them & (knights))          ||
      (KING_ATTACKS   (idx) & them & (kings)))
    return true;

  // Are we attacked by a pawn?
  bitboard attacks;
  bitboard their_pawns = pawns & them;
  if (c != WHITE)
    {
      attacks = (((their_pawns & ~file_mask (H)) >> 7)
                 | ((their_pawns & ~file_mask (A)) >> 9));
    }
  else
    {
      attacks = (((their_pawns & ~file_mask (A)) << 7)
                 | ((their_pawns & ~file_mask (H)) << 9));
    }

  if (attacks & masks_0[idx]) return true;

  return false;
}

// For the currently moving side, find the least valuable piece
// attacking a square. The strategy here is to compute the set of
// locations an attack could be coming from, and then taking the union
// of that set and pieces actually at those locations. En Passant is
// ignored for now.
Move
Board::least_valuable_attacker (int sqr) const {
  Color c = to_move ();
  bitboard us = color_to_board (c);
  bitboard from;

  // Pawns.
  bitboard our_pawns = pawns & us;
  if (c == WHITE)
    {
      from = ((masks_0 [sqr] & ~file_mask (H)) >> 7) & our_pawns;
      if (from) return Move (bit_idx (from), sqr);
      from = ((masks_0 [sqr] & ~file_mask (A)) >> 9) & our_pawns;
      if (from) return Move (bit_idx (from), sqr);
    }
  else
    {
      from = ((masks_0 [sqr] & ~file_mask (A)) << 7) & our_pawns;
      if (from) return Move (bit_idx (from), sqr);
      from = ((masks_0 [sqr] & ~file_mask (H)) << 9) & our_pawns;
      if (from) return Move (bit_idx (from), sqr);
    }

  // Knights.
  from = KNIGHT_ATTACKS (sqr) & (knights & us);
  if (from) return Move (bit_idx (from), sqr);

  // Bishops.
  from = BISHOP_ATTACKS (sqr) & (bishops & us);
  if (from) return Move (bit_idx (from), sqr);

  // Rooks.
  from = ROOK_ATTACKS (sqr) & (rooks & us);
  if (from) return Move (bit_idx (from), sqr);

  // Queens
  from = QUEEN_ATTACKS (sqr) & (queens & us);
  if (from) return Move (bit_idx (from), sqr);

  // Kings
  from = KING_ATTACKS_TBL[sqr] & (kings & us);
  if (from) return Move (bit_idx (from), sqr);

  // Return a null move if no piece is attacking this square.
  return Move (0, 0);
}

// Return whether color c is in check.
bool
Board::in_check (Color c) const
{
  int idx = bit_idx (kings & color_to_board (c));
  assert (idx >= 0 && idx < 64);
  return is_attacked (idx, invert (c));
}

// Generate the number of moves available at ply d. Used for debugging
// the move generator.
uint64
Board::perft (int d) const {
  uint64 sum = 0;

  if (d == 0) return 1;

  Move_Vector moves (*this);
  for (int i = 0; i < moves.count; i++)
    {
      Board c (*this);
      if (c.apply (moves[i])) sum += c.perft (d - 1);
    }
  return sum;
}

// For each child, print the child move and the perft (d) of the
// resulting board.
void
Board :: divide (int d) const {
  Move_Vector moves (*this);

  for (int i = 0; i < moves.count; i++)
    {
      Board child = *this;
      std::cerr << to_calg (moves[i]) << " ";
      if (child.apply (moves [i]))
        std::cerr << child.perft (d - 1) << std::endl;
    }
}
