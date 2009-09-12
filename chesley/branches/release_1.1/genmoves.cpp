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
      coord from_idx = bit_idx (from);
      while (to)
        {
          coord to_idx = bit_idx (to);

          // Handle the case of a promotion.
          if ((c == WHITE && idx_to_rank (to_idx) == 7) ||
              (c == BLACK && idx_to_rank (to_idx) == 0))
            {
              // Generate a move for each possible promotion.
              for (Kind k = ROOK; k <= QUEEN; k++)
                {
                  moves.push (from_idx, to_idx, to_move (), 
                              PAWN, get_kind (to_idx), k);
                }
            }
          else
            {
              Kind capture = get_kind (to_idx);
              if (idx_to_file (from_idx) != idx_to_file (to_idx) && 
                  capture == NULL_KIND)
                {
                  // Handle the En Passant case.
                  moves.push (from_idx, to_idx, to_move (), 
                              PAWN, PAWN, NULL_KIND, true);
                }
              else
                {
                  moves.push (from_idx, to_idx, to_move (), 
                              PAWN, capture);
                }
            }
          clear_bit (to, to_idx);
        }

      clear_bit (our_pawns, from_idx);
    }

  ////////////
  // Rooks. //
  ////////////

  bitboard our_rooks = rooks & our_pieces ();

  // For each rook
  while (our_rooks)
    {
      coord from = bit_idx (our_rooks);

      // Collect each destination in the moves list.
      bitboard to = rook_attacks (from) & ~our_pieces ();
      while (to)
        {
          coord to_idx = bit_idx (to);
          moves.push (from, to_idx, to_move (), ROOK, get_kind (to_idx));
          clear_bit (to, to_idx);
        }

      clear_bit (our_rooks, from);
    }

  //////////////
  // Knights. //
  //////////////

  bitboard our_knights = knights & our_pieces ();

  // For each knight:
  while (our_knights)
    {
      coord from = bit_idx (our_knights);

      // Collect each destination in the moves list.
      bitboard to = knight_attacks (from) & ~our_pieces ();
      while (to)
        {
          coord to_idx = bit_idx (to);
          moves.push (from, to_idx, to_move (), KNIGHT, get_kind (to_idx));
          clear_bit (to, to_idx);
        }
      clear_bit (our_knights, from);
    }

  //////////////
  // Bishops. //
  //////////////

  bitboard our_bishops = bishops & our_pieces ();

  // For each bishop;
  while (our_bishops)
    {
      coord from = bit_idx (our_bishops);

      // Collect each destination in the moves list.
      bitboard to = bishop_attacks (from) & ~our_pieces ();
      while (to)
        {
          coord to_idx = bit_idx (to);
          moves.push (from, to_idx, to_move (), BISHOP, get_kind (to_idx));
          clear_bit (to, to_idx);
        }
      clear_bit (our_bishops, from);
    }

  /////////////
  // Queens. //
  /////////////

  bitboard our_queens = queens & our_pieces ();

  // For each queen.
  while (our_queens)
    {
      coord from = bit_idx (our_queens);

      // Collect each destination in the moves list.
      bitboard to = queen_attacks (from) & ~our_pieces ();
      while (to)
        {
          coord to_idx = bit_idx (to);
          moves.push (from, to_idx, to_move (), QUEEN, get_kind (to_idx));
          clear_bit (to, to_idx);
        }
      clear_bit (our_queens, from);
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
      coord from = bit_idx (our_king);

      // Collect each destination in the moves list.
      bitboard to = king_attacks (from) & ~our_pieces ();
      while (to)
        {
          coord to_idx = bit_idx (to);
          moves.push (from, to_idx, to_move (), KING, get_kind (to_idx));
          clear_bit (to, to_idx);
        }
    }

  /////////////////////////////
  // Compute castling moves. //
  /////////////////////////////

  if (c == WHITE)
    {
      byte row = occ_0 (E1);

      if (flags.w_can_k_castle && !(row & 0x60))
        {
          moves.push (E1, G1, c, KING, NULL_KIND);
        }

      if (flags.w_can_q_castle && !(row & 0xE))
        {
          moves.push (E1, C1, c, KING, NULL_KIND);
        }
    }
  else
    {
      byte row = occ_0 (E8);

      if (flags.b_can_k_castle && !(row & 0x60))
        {
          moves.push (E8, G8, c, KING, NULL_KIND);
        }

      if (flags.b_can_q_castle && !(row & 0xE))
        {
          moves.push (E8, C8, c, KING, NULL_KIND);
        }
    }
}

// Generate non-capture promotions to Queen.
void
Board::gen_promotions (Move_Vector &moves) const
{
  Color c = to_move ();
  bitboard our_pawns = pawns & our_pieces ();

  // Select the pawns on the 2nd or 7th rank with an empty square
  // ahead of them.
  if (c == WHITE) 
    {
      our_pawns &= rank_mask (6);
      our_pawns = ((our_pawns << 8) & unoccupied ()) >> 8;
    }
  else 
    {
      our_pawns &= rank_mask (1);
      our_pawns = ((our_pawns >> 8) & unoccupied ()) << 8;
    }

  // For each pawn:
  while (our_pawns)
    {
      coord from = bit_idx (our_pawns);
      coord to = (c == WHITE) ? (from + 8) : (from - 8);
      moves.push (from, to, c, PAWN, NULL_KIND, QUEEN);
      clear_bit (our_pawns, from);
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
      coord from_idx = bit_idx (from);
      while (to)
        {
          coord to_idx = bit_idx (to);
          Kind capture = get_kind (to_idx);
          if (idx_to_file (from_idx) != idx_to_file (to_idx) && 
              capture == NULL_KIND)
            {
              // Handle the En Passant case.
              moves.push (from_idx, to_idx, to_move (), 
                          PAWN, PAWN, NULL_KIND, true);
            }
          else
            {
              moves.push (from_idx, to_idx, to_move (), 
                          PAWN, capture);
            }
          clear_bit (to, to_idx);
        }
      clear_bit (our_pawns, from_idx);
    }

  /////////////////////////////
  // Generate rook captures. //
  /////////////////////////////

  bitboard our_rooks = rooks & our_pieces ();

  // For each rook
  while (our_rooks)
    {
      coord from = bit_idx (our_rooks);

      // Collect each destination in the moves list.
      bitboard to = rook_attacks (from) & ~our_pieces ();
      to &= other_pieces ();
      while (to)
        {
          coord to_idx = bit_idx (to);
          moves.push (from, to_idx, to_move (), ROOK, get_kind (to_idx));
          clear_bit (to, to_idx);
        }

      clear_bit (our_rooks, from);
    }

  ///////////////////////////////
  // Generate knight captures. //
  ///////////////////////////////

  bitboard our_knights = knights & our_pieces ();

  // For each knight:
  while (our_knights)
    {
      coord from = bit_idx (our_knights);

      // Collect each destination in the moves list.
      bitboard to = knight_attacks (from) & ~our_pieces ();
      to &= other_pieces ();
      while (to)
        {
          coord to_idx = bit_idx (to);
          moves.push (from, to_idx, to_move (), KNIGHT, get_kind (to_idx));
          clear_bit (to, to_idx);
        }
      clear_bit (our_knights, from);
    }

  ///////////////////////////////
  // Generate bishop captures. //
  ///////////////////////////////

  bitboard our_bishops = bishops & our_pieces ();

  // For each bishop;
  while (our_bishops)
    {
      // Look up destinations in moves table and collect each in 'moves'.
      coord from = bit_idx (our_bishops);

      // Collect each destination in the moves list.
      bitboard to = bishop_attacks (from) & ~our_pieces ();
      to &= other_pieces ();
      while (to)
        {
          coord to_idx = bit_idx (to);
          moves.push (from, to_idx, to_move (), BISHOP, get_kind (to_idx));
          clear_bit (to, to_idx);
        }
      clear_bit (our_bishops, from);
    }

  //////////////////////////////
  // Generate queen captures. //
  //////////////////////////////

  bitboard our_queens = queens & our_pieces ();

  // For each queen.
  while (our_queens)
    {
      // Look up destinations in moves table and collect each in 'moves'.
      coord from = bit_idx (our_queens);

      // Collect each destination in the moves list.
      bitboard to = queen_attacks (from);
      to &= other_pieces ();
      while (to)
        {
          coord to_idx = bit_idx (to);
          moves.push (from, to_idx, to_move (), QUEEN, get_kind (to_idx));
          clear_bit (to, to_idx);
        }
      clear_bit (our_queens, from);
    }

  /////////////////////////////
  // Generate king captures. //
  /////////////////////////////

  bitboard our_king = kings & our_pieces ();

  if (our_king)
    {
      coord from = bit_idx (our_king);

      // Collect each destination in the moves list.
      bitboard to = king_attacks (from) & ~our_pieces ();
      to &= other_pieces ();
      while (to)
        {
          coord to_idx = bit_idx (to);
          moves.push (from, to_idx, to_move (), KING, get_kind (to_idx));
          clear_bit (to, to_idx);
        }
    }
}

// Compute a bitboard of every square color is attacking.
bitboard
Board::attack_set (Color c) const {
  bitboard color = c == WHITE ? white : black;
  bitboard attacks;
  bitboard pieces;
  coord from;

  // Pawns
  attacks = get_pawn_attacks (c);

  // Rooks
  pieces = rooks & color;
  while (pieces)
    {
      from = bit_idx (pieces);
      attacks |= rook_attacks (from);
      clear_bit (pieces, from);
    }

  // Knights
  pieces = knights & color;
  while (pieces)
    {
      from = bit_idx (pieces);
      attacks |= knight_attacks (from);
      clear_bit (pieces, from);
    }

  // Bishops
  pieces = bishops & color;
  while (pieces)
    {
      from = bit_idx (pieces);
      attacks |= bishop_attacks (from);
      clear_bit (pieces, from);
    }

  // Queens
  pieces = queens & color;
  while (pieces)
    {
      from = bit_idx (pieces);
      attacks |= queen_attacks (from);
      clear_bit (pieces, from);
    }

  // Kings
  pieces = kings & color;
  while (pieces)
    {
      from = bit_idx (pieces);
      attacks |= king_attacks (from);
      clear_bit (pieces, from);
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
Board::is_attacked (coord idx, Color c) const
{
  // Take advantage of the symmetry that if the piece at index could
  // move like an X and capture an X, then that X is able to attack it
  bitboard them = color_to_board (c);
  return ((rook_attacks   (idx) & them & (queens | rooks))   ||
          (bishop_attacks (idx) & them & (queens | bishops)) ||
          (knight_attacks (idx) & them & (knights))          ||
          (king_attacks   (idx) & them & (kings))            ||
          (get_pawn_attacks (c) & masks_0[idx]));
}

// For the currently moving side, find the least valuable piece
// attacking a square. The strategy here is to compute the set of
// locations an attack could be coming from, and then taking the union
// of that set and pieces actually at those locations. En Passant is
// ignored for now.
Move
Board::least_valuable_attacker (coord sqr) const {
  Color c = to_move ();
  bitboard us = color_to_board (c);
  bitboard from;

  // Pawns.
  bitboard our_pawns = pawns & us;
  if (c == WHITE)
    {
      from = ((masks_0 [sqr] & ~file_mask (H)) >> 7) & our_pawns;
      if (from) return Move (bit_idx (from), sqr, c, PAWN, get_kind (sqr));
      from = ((masks_0 [sqr] & ~file_mask (A)) >> 9) & our_pawns;
      if (from) return Move (bit_idx (from), sqr, c, PAWN, get_kind (sqr));
    }
  else
    {
      from = ((masks_0 [sqr] & ~file_mask (A)) << 7) & our_pawns;
      if (from) return Move (bit_idx (from), sqr, c, PAWN, get_kind (sqr));
      from = ((masks_0 [sqr] & ~file_mask (H)) << 9) & our_pawns;
      if (from) return Move (bit_idx (from), sqr, c, PAWN, get_kind (sqr));
    }

  // Knights.
  from = knight_attacks (sqr) & (knights & us);
  if (from) return Move (bit_idx (from), sqr, c, KNIGHT, get_kind (sqr));

  // Bishops.
  from = bishop_attacks (sqr) & (bishops & us);
  if (from) return Move (bit_idx (from), sqr, c, BISHOP, get_kind (sqr));

  // Rooks.
  from = rook_attacks (sqr) & (rooks & us);
  if (from) return Move (bit_idx (from), sqr, c, ROOK, get_kind (sqr));

  // Queens
  from = queen_attacks (sqr) & (queens & us);
  if (from) return Move (bit_idx (from), sqr, c, QUEEN, get_kind (sqr));

  // Kings
  from = king_attacks (sqr) & (kings & us);
  if (from) return Move (bit_idx (from), sqr, c, KING, get_kind (sqr));

  // Return a null move if no piece is attacking this square.
  return NULL_MOVE;
}

// Return whether color c is in check.
bool
Board::in_check (Color c) const
{
  coord idx = bit_idx (kings & color_to_board (c));

  if (idx >= 64)
    {
      cout << to_fen () << endl;
      cout << kings << endl;
      cout << c << endl;
      cout << (kings & color_to_board (c)) << endl;
      cout << idx << endl;
      assert (0);
    }

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
