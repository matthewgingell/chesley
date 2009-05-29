////////////////////////////////////////////////////////////////////////////////
// board.cpp                                                                  //
//                                                                            //
// Representation and operations on a board state in a game of chess.         //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <cassert>
#include <cctype>
#include <sstream>

#include "chesley.hpp"

using namespace std;

////////////////
// Kind type. //
////////////////

// Convert a kind to a character code.
char
to_char (Kind k) {
  switch (k)
    {
    case NULL_KIND: return '?';
    case PAWN:      return 'P';
    case ROOK:      return 'R';
    case KNIGHT:    return 'N';
    case BISHOP:    return 'B';
    case QUEEN:     return 'Q';
    case KING:      return 'K';
    }
  assert (0);

  // Suppress gcc warning in -DNDEBUG case.
  return '!';
}

// Convert a character code to a piece code, ignoring color.
Kind
to_kind (char k) {
  k = toupper (k);
  switch (k)
    {
      case 'P': return PAWN;
      case 'R': return ROOK;
      case 'N': return KNIGHT;
      case 'B': return BISHOP;
      case 'Q': return QUEEN;
      case 'K': return KING;
    }
  assert (0);
  return NULL_KIND;
}

std::ostream &
operator<< (std::ostream &os, Kind k) {
    switch (k)
    {
    case NULL_KIND: return os << "NULL_KIND";
    case PAWN:      return os << "PAWN";
    case ROOK:      return os << "ROOK";
    case KNIGHT:    return os << "KNIGHT";
    case BISHOP:    return os << "BISHOP";
    case QUEEN:     return os << "QUEEN";
    case KING:      return os << "KING";
    default:        cerr << k << endl; assert (0);
    }

    // Suppress gcc warning in -DNDEBUG case.
    return os;
}

/////////////////
// Color type. //
/////////////////

std::ostream &
operator<< (std::ostream &os, Color c) {
  switch (c)
    {
    case WHITE: return os << "WHITE";
    case BLACK: return os << "BLACK";
    default: return os;
    }
}

////////////////
// Constants. //
////////////////

const string Board::
INITIAL_POSITIONS = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

///////////////////
// Constructors. //
///////////////////

void
Board::common_init (Board &b) {

  // Initialize bitboards.
  b.white = b.black = 0x0;
  b.pawns = b.rooks = b.knights = b.bishops = b.queens = b.kings = 0x0;
  b.occupied = b.occupied_45 = b.occupied_90 = b.occupied_135 = 0x0;

  // Set side to move.
  b.flags.to_move = WHITE;

  // Initialize castling status
  b.flags.w_has_q_castled  = 0;
  b.flags.w_has_k_castled  = 0;
  b.flags.w_can_q_castle   = 1;
  b.flags.w_can_k_castle   = 1;
  b.flags.b_has_q_castled  = 0;
  b.flags.b_has_k_castled  = 0;
  b.flags.b_can_q_castle   = 1;
  b.flags.b_can_k_castle   = 1;

  // Initialize en_passant status.
  b.flags.en_passant = 0;

  // Set up initial hash value.
  b.hash = 0x0;

  // White is to move.
  b.hash ^= zobrist_key_white_to_move;

  // All castling moves are possible.
  b.hash ^= zobrist_w_castle_q_key;
  b.hash ^= zobrist_w_castle_k_key;
  b.hash ^= zobrist_b_castle_q_key;
  b.hash ^= zobrist_b_castle_k_key;

  // The en passant square is not set.
  b.hash ^= zobrist_enpassant_keys[0];

  // Initialize clocks.
  b.half_move_clock =
    b.full_move_clock = 0;
}

// Construct a board from the standard starting position.
Board
Board::startpos () {
  return Board::from_fen
    ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

/////////////////////////////
// Seting castling rights. //
/////////////////////////////

void
Board::set_castling_right (Castling_Right cr, bool v) {
  switch (cr)
    {
    case W_QUEEN_SIDE:
      if (flags.w_can_q_castle != v)
	{
	  hash ^= zobrist_w_castle_q_key;
	  flags.w_can_q_castle = v;
	}
      break;

    case W_KING_SIDE:
      if (flags.w_can_k_castle != v)
	{
	  hash ^= zobrist_w_castle_k_key;
	  flags.w_can_k_castle = v;
	}
      break;

    case B_QUEEN_SIDE:
      if (flags.b_can_q_castle != v)
	{
	  hash ^= zobrist_b_castle_q_key;
	  flags.b_can_q_castle = v;
	}
      break;

    case B_KING_SIDE:
      if (flags.b_can_k_castle != v)
	{
	  hash ^= zobrist_b_castle_k_key;
	  flags.b_can_k_castle = v;
	}
      break;
    }
}

///////////////////////
// Move application. //
///////////////////////

// Apply a move to the board. Return false if this move is illegal
// because it places or leaves the color to move in check.
bool
Board::apply (const Move &m) {
  Kind kind = m.get_kind (*this);
  Kind capture = m.capture (*this);
  Color color = to_move ();

  ////////////////////
  // Update clocks. //
  ////////////////////

  // Reset the 50 move rule clock when a pawn is moved or a capture is
  // made.
  if (capture != NULL_KIND || kind == PAWN)
    {
      half_move_clock = 0;
    }
  else
    {
      half_move_clock++;
    }

  // Increment the whole move clock after each move by black.
  if (color == BLACK) full_move_clock++;

  // There is no legal move with a half move clock greater than 50.
  if (half_move_clock > 50) return false;

  ///////////////////////////////
  // Handle taking En Passant. //
  ///////////////////////////////

  if (kind == PAWN)
    {
      if (m.is_en_passant (*this))
	{
	  // Clear the square behind destination square.
	  if (color == WHITE) clear_piece (flags.en_passant - 8);
	  else clear_piece (flags.en_passant + 8);
	}

      ///////////////////////////////////////////////////////////////////
      // Update En Passant target square. This needs to be done before //
      // checking castling, since otherwise we may return without      //
      // clearing the En Passant target square.                        //
      ///////////////////////////////////////////////////////////////////

      if (color == WHITE)
	{
	  if ((idx_to_rank (m.from) == 1) && (idx_to_rank (m.to) == 3))
	    {
	      set_en_passant (m.from + 8);
	    }
	  else
	    {
	      set_en_passant (0);
	    }
	}
      else
	{
	  if ((idx_to_rank (m.from) == 6) && (idx_to_rank (m.to) == 4))
	    {
	      set_en_passant (m.from - 8);
	    }
	  else
	    {
	      set_en_passant (0);
	    }
	}
    }
  else
    {
      set_en_passant (0);
    }

  //////////////////////////////
  // Handling castling moves. //
  //////////////////////////////

  if (kind == KING)
    {
      bool is_castle_qs = m.is_castle_qs (*this);
      bool is_castle_ks = m.is_castle_ks (*this);
      if (is_castle_qs || is_castle_ks)
	{
	  // Calculate attacked set for testing check.
	  const bitboard attacked = attack_set (invert (color));

	  // Test castle out of, across, or in to check.
	  if (color == WHITE)
	    {
	      if (is_castle_qs && !(attacked & 0x1C))
		{
		  clear_piece (E1);
		  clear_piece (A1);
		  set_piece (KING, WHITE, C1);
		  set_piece (ROOK, WHITE, D1);
		  set_castling_right (W_QUEEN_SIDE, false);
		  set_castling_right (W_KING_SIDE, false);
		  set_color (invert (to_move ()));
  		  flags.w_has_k_castled = 1;
		  return true;
		}
	      if (is_castle_ks && !(attacked & 0x70))
		{
		  clear_piece (E1);
		  clear_piece (H1);
		  set_piece (KING, WHITE, G1);
		  set_piece (ROOK, WHITE, F1);
		  set_castling_right (W_QUEEN_SIDE, false);
		  set_castling_right (W_KING_SIDE, false);
		  set_color (invert (to_move ()));
		  flags.w_has_k_castled = 1;
		  return true;
		}
	    }
	  else
	    {
	      byte attacks = get_byte (attacked, 7);
	      if (is_castle_qs && !(attacks & 0x1C))
		{
		  clear_piece (E8);
		  clear_piece (A8);
		  set_piece (KING, BLACK, C8);
		  set_piece (ROOK, BLACK, D8);
		  set_castling_right (B_QUEEN_SIDE, false);
		  set_castling_right (B_KING_SIDE, false);
		  set_color (invert (to_move ()));
		  flags.b_has_q_castled = 1;
		  return true;
		}
	      if (is_castle_ks && !(attacks & 0x70))
		{
		  clear_piece (E8);
		  clear_piece (H8);
		  set_piece (KING, BLACK, G8);
		  set_piece (ROOK, BLACK, F8);
		  set_castling_right (B_QUEEN_SIDE, false);
		  set_castling_right (B_KING_SIDE, false);
		  set_color (invert (to_move ()));
		  flags.b_has_k_castled = 1;
		  return true;
		}
	    }

	  return false;
	}
    }

  ///////////////////////////////////////////////
  // Clear the origin and destination squares. //
  ///////////////////////////////////////////////

  ///////////////////////////
  // Update to_move state. //
  ///////////////////////////

  set_color (invert (to_move ()));
  clear_piece (m.from);
  clear_piece (m.to);

  /////////////////////////////////////////////////////
  // Set the destination square, possibly promoting. //
  /////////////////////////////////////////////////////

  if (m.promote != NULL_KIND)
    {
      set_piece (m.promote, color, m.to);
    }
  else
    {
      set_piece (kind, color, m.to);
    }

  /////////////////////////////
  // Update castling status. //
  /////////////////////////////

  // King moves.
  if (m.from == E1)
    {
      set_castling_right (W_QUEEN_SIDE, false);
      set_castling_right (W_KING_SIDE, false);
    }
  else if (m.from == E8)
    {
      set_castling_right (B_QUEEN_SIDE, false);
      set_castling_right (B_KING_SIDE, false);
    }

  // Rook moves and attacks.
  if (m.from == A1 || m.to == A1) { set_castling_right (W_QUEEN_SIDE, false); }
  if (m.from == H1 || m.to == H1) { set_castling_right (W_KING_SIDE, false);  }
  if (m.from == A8 || m.to == A8) { set_castling_right (B_QUEEN_SIDE, false); }
  if (m.from == H8 || m.to == H8) { set_castling_right (B_KING_SIDE, false);  }

  ////////////////////
  // Test legality. //
  ////////////////////

  // Return whether this position puts or leaves us in check.
  return !in_check (color);
}

/////////////
//   I/O   //
/////////////

// Print a board.
ostream &
operator<< (ostream &os, const Board &b)
{
  return os << b.to_ascii ();
}

// Print a move.
std::ostream &
operator<< (std::ostream &os, const Move &m)
{
  return os << "[Move from "
	    << (int) (m.from) << " => " << (int) (m.to)
	    << " " << m.score << "]";
}

//////////////
// Testing. //
//////////////

// Print a 64 bit set as a 8x8 matrix of 'X; and '.'.
void
print_board (bitboard b) {
  for (int y = 7; y >= 0; y--)
    {
      for (int x = 0; x < 8; x++)
        cerr << (test_bit (b, x + 8 * y) ? 'X' : '.') << " ";
      cerr << endl;
    }
  cerr << endl;
}

// Print the full tree to depth N.
void
Board::print_tree (int depth)
{
  if (depth == 0)
    cerr<< *this << endl;
  else
    {
      Move_Vector moves (*this);
      for (int i = 0; i < moves.count; i++)
	{
	  Board c = *this;
	  if (c.apply (moves[i])) c.print_tree (depth - 1);
	}
    }
}

// Generate a hash key from scratch. This is used to test the
// correctness of our incremental hash update code.
uint64
Board::gen_hash () const {
  uint64 h = 0x0;

  // Set color to move.
  if (to_move () == WHITE) h ^= zobrist_key_white_to_move;

  // Set castling rights.
  if (flags.w_can_q_castle) h ^= zobrist_w_castle_q_key;
  if (flags.w_can_k_castle) h ^= zobrist_w_castle_k_key;
  if (flags.b_can_q_castle) h ^= zobrist_b_castle_q_key;
  if (flags.b_can_k_castle) h ^= zobrist_b_castle_k_key;

  // Set en passant status.
  h ^= zobrist_enpassant_keys[flags.en_passant];

  // Set all pieces.
  for (int i = 0; i < 64; i++)
    {
      Kind k = get_kind (i);
      Color c = get_color (i);
      if (k != NULL_KIND && c != NULL_COLOR)
	{
	  h ^= Board::get_zobrist_piece_key (c, k, i);
	}
    }

  return h;
}
