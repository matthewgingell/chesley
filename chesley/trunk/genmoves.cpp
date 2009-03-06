/*
  genmoves.cpp

  Code to generate a vector of moves, given a state of play.

  There is a good discusion of generating moves from bitmaps in:
  _Rotated bitmaps, a new twist on an old idea_ by Robert Hyatt at
  http://www.cis.uab.edu/info/faculty/hyatt/bitmaps.html.

  The code here is very sensitive and changes should be checked
  against the perft suite to ensure they do not cause regressions.

  Matthew Gingell
  gingell@adacore.com
*/

#include "chesley.hpp"

// Collect all possible moves.
void
Board::gen_all_moves (Move_Vector &out) const
{
  assert (have_precomputed_tables);

  gen_pawn_moves (out);
  gen_rook_moves (out);
  gen_knight_moves (out);
  gen_bishop_moves (out);
  gen_queen_moves (out);
  gen_king_moves (out);
}

// Generate the number of moves available at ply d. Used for debugging
// the move generator.
uint64
Board::perft (int d) const {
  uint64 sum = 0;

  if (d == 0) return 1;
  Board_Vector children (*this);
  for (int i = 0; i < children.count; i++)
    sum += children[i].perft (d - 1);
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

/*********/
/* Pawns */
/*********/

inline void
Board::gen_pawn_moves (Move_Vector &out) const
{
  Color c = flags.to_move;
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
	  to |= ((to & rank (2)) << 8) & unoccupied ();

	  // Capture forward right.
	  to |= ((from & ~file(0)) << 7) & black;

	  // Capture forward left.
	  to |= ((from & ~file(7)) << 9) & black;

	  // Pawns which can reach the En Passant square.
	  if (flags.en_passant != 0 &&
	      ((bit_idx ((from & ~file(0)) << 7) == flags.en_passant) ||
	       (bit_idx ((from & ~file(7)) << 9) == flags.en_passant)))
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
	  to |= ((to & rank (5)) >> 8) & unoccupied ();

	  // Capture forward left.
	  to |= ((from & ~file(7)) >> 7) & white;

	  // Capture forward right.
	  to |= ((from & ~file(0)) >> 9) & white;

	  // Pawns which can reach the En Passant square.
	  if (flags.en_passant != 0 &&
	      ((bit_idx ((from & ~file(7)) >> 7) == flags.en_passant) ||
	       (bit_idx ((from & ~file(0)) >> 9) == flags.en_passant)))
	    {
	      to |= masks_0[flags.en_passant];
	    }
	}

      // Collect each destination in the moves list.
      while (to)
	{
	  int to_idx = bit_idx (to);

	  Move m (bit_idx (from), to_idx);

	  // Handle the case of a promotion.
	  if ((c == WHITE && idx_to_rank (to_idx) == 7) ||
	      (c == BLACK && idx_to_rank (to_idx) == 0))
	    {
	      // Generate a move for each possible promotion.
	      for (int k = (int) ROOK; k <= (int) QUEEN; k++)
		{
		  m.promote = (Kind) k;
		  out.push (m);
		}
	    }
	  else
	    {
	      // Handle non-promotion case;
	      out.push (m);
	    }

	  to = clear_lsb (to);
	}

      our_pawns = clear_lsb (our_pawns);
    }
}

/*********/
/* Rooks */
/*********/

inline void
Board::gen_rook_moves (Move_Vector &out) const
{
  bitboard our_rooks = rooks & our_pieces ();

  // For each rook
  while (our_rooks)
    {
      int from = bit_idx (our_rooks);

      bitboard to =
	(RANK_ATTACKS_TBL[from * 256 + occ_0 (from)] |
	 FILE_ATTACKS_TBL[from * 256 + occ_90 (from)])
	& ~our_pieces ();

      // Collect each destination in the moves list.
      while (to)
	{
	  int to_idx = bit_idx (to);
	  out.push (Move (from,  to_idx));
	  to = clear_lsb (to);
	}

      our_rooks = clear_lsb (our_rooks);
    }
}

/***********/
/* Knights */
/***********/

inline void
Board::gen_knight_moves (Move_Vector &out) const
{
  bitboard our_knights = knights & our_pieces ();

  // For each knight:
  while (our_knights)
    {
      int from = bit_idx (our_knights);
      bitboard to = KNIGHT_ATTACKS_TBL[from] & ~our_pieces ();

      // Collect each destination in the moves list.
      while (to)
	{
	  int to_idx = bit_idx (to);
	  out.push (Move (from, to_idx));
	  to = clear_lsb (to);
	}
      our_knights = clear_lsb (our_knights);
    }
}


/***********/
/* Bishops */
/***********/

inline void
Board::gen_bishop_moves (Move_Vector &out) const
{
  bitboard our_bishops = bishops & our_pieces ();

  // For each bishop;
  while (our_bishops)
    {
      // Look up destinations in moves table and collect each in 'out'.
      int from = bit_idx (our_bishops);
      bitboard to;

      to = (DIAG_45_ATTACKS_TBL[256 * from + occ_45 (from)] |
	    DIAG_135_ATTACKS_TBL[256 * from + occ_135 (from)])
	& ~our_pieces ();

      // Collect each destination in the moves list.
      while (to)
	{
	  int to_idx = bit_idx (to);
	  out.push (Move (from, to_idx));
	  to = clear_lsb (to);
	}
      our_bishops = clear_lsb (our_bishops);
    }
}

/**********/
/* Queens */
/**********/

inline void
Board::gen_queen_moves (Move_Vector &out) const
{
  bitboard our_queens = queens & our_pieces ();

  // For each queen.
  while (our_queens)
    {
      // Look up destinations in moves table and collect each in 'out'.
      int from = bit_idx (our_queens);

      bitboard to =
	(RANK_ATTACKS_TBL[256 * from + occ_0 (from)]
	 | FILE_ATTACKS_TBL[256 * from + occ_90 (from)]
	 | DIAG_45_ATTACKS_TBL[256 * from + occ_45 (from)]
	 | DIAG_135_ATTACKS_TBL[256 * from + occ_135 (from)])
	& ~our_pieces ();

      // Collect each destination in the moves list.
      while (to)
	{
	  int to_idx = bit_idx (to);
	  out.push (Move (from,  to_idx));
	  to = clear_lsb (to);
	}
      our_queens = clear_lsb (our_queens);
    }
}


/*********/
/* Kings */
/*********/

inline void
Board::gen_king_moves (Move_Vector &out) const
{
  Color c = flags.to_move;
  bitboard our_king = kings & our_pieces ();

  /*************************/
  /* Compute simple moves. */
  /*************************/

  if (our_king)
    {
      int from = bit_idx (our_king);
      bitboard to = KING_ATTACKS_TBL[from] & ~our_pieces ();

      // Collect each destination in the moves list.
      while (to)
	{
	  int to_idx = bit_idx (to);
	  out.push (Move (from,  to_idx));
	  to = clear_lsb (to);
	}
    }

  /**************************/
  /* Compute castling moves */
  /**************************/

  if (c == WHITE)
    {
      byte row = occ_0 (4);

      if (flags.w_can_q_castle && (row & 0xE) == 0)
	out.push (Move (4, 2));


      if (flags.w_can_k_castle && (row & 0x60) == 0)
	out.push (Move (4, 6));
    }
  else
    {
      byte row = occ_0 (60);
      
      if (flags.b_can_q_castle && (row & 0xE) == 0)
	out.push (Move (60, 58));
      
      if (flags.b_can_k_castle && (row & 0x60) == 0)
	out.push (Move (60, 62));
    }
}
