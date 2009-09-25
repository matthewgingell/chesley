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

///////////////
// Kind type //
///////////////

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
    default:        assert (0);
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
  // assert (0);
  throw string ("Failure in to_kind");
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

////////////////
// Color type //
////////////////

std::ostream &
operator<< (std::ostream &os, Color c) {
  switch (c)
    {
    case WHITE: return os << "WHITE";
    case BLACK: return os << "BLACK";
    default: return os;
    }
}

///////////////
// Constants //
///////////////

const string
INITIAL_POSITIONS = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

//////////////////
// Constructors //
//////////////////

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

  // Set up initial hash values.
  b.hash = 0x0;
  b.phash = 0x0;

  // White is to move.
  b.hash ^= zobrist_key_white_to_move;
  b.phash ^= zobrist_key_white_to_move;

  // All castling moves are possible.
  b.hash ^= zobrist_w_castle_q_key;
  b.hash ^= zobrist_w_castle_k_key;
  b.hash ^= zobrist_b_castle_q_key;
  b.hash ^= zobrist_b_castle_k_key;

  // The en passant square is not set.
  b.hash ^= zobrist_enpassant_keys[0];

  // Initialize clocks.
  b.half_move_clock = b.full_move_clock = 0;

  // Initialize scoring information.
  b.net_material = 0;
  ZERO (b.psquares);
  ZERO (b.piece_counts);
  ZERO (b.pawn_counts);
}

// Construct a board from the standard starting position.
Board
Board::startpos () {
  return Board::from_fen
    ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

/////////////////////////////
// Setting castling rights //
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

//////////////////////////////////////
// Setting and updating the board.  //
//////////////////////////////////////

// Clear a piece on the board.
void
Board::clear_piece (Coord idx) {
  if (occupied & masks_0[idx])
    {
      Color c = get_color (idx);
      Kind k = get_kind (idx);
      clear_piece (k, c, idx);
    }
}

void
Board::clear_piece (Kind k, Color c, Coord idx) {
  if (occupied & masks_0[idx])
    {
      assert (k != NULL_KIND);
      assert (c == BLACK || c == WHITE);
      assert (idx < 64);

      // Clear color and piece kind bits.
      const bits64 mask = 1ULL << idx;
      white &= ~mask;
      black &= ~mask;
      kind_to_board (k) &= ~mask;

      // Update hash key.
      hash ^= get_zobrist_piece_key (c, k, idx);

      // Update evaluation information.
      net_material -= sign (c) * Eval::eval_piece (k);
      psquares[c] -= Eval::psq_value (k, c, idx);
      piece_counts[c][k]--;
      
      // Check the special case of clearing a pawn.
      if (k == PAWN) 
        {
          phash ^= get_zobrist_piece_key (c, k, idx);
          pawn_counts[c][idx_to_file (idx)]--;
        }

      // Clear the occupancy sets.
      occupied     &= ~masks_0[idx];
      occupied_45  &= ~masks_45[idx];
      occupied_90  &= ~masks_90[idx];
      occupied_135 &= ~masks_135[idx];
    }
}

// Set a piece on the board with an index.
void
Board::set_piece (Kind k, Color c, Coord idx) {
  assert (~occupied & masks_0[idx]);
  assert (k != NULL_KIND);
  assert (c == BLACK || c == WHITE);
  assert (idx < 64);

  // Update color and piece sets.
  color_to_board (c) |= masks_0[idx];
  kind_to_board (k) |= masks_0[idx];

  // Update hash key.
  hash ^= get_zobrist_piece_key (c, k, idx);
  if (k == PAWN) phash ^= get_zobrist_piece_key (c, k, idx);

  // Update evaluation information.
  net_material += sign (c) * Eval::eval_piece (k);
  psquares[c] += Eval::psq_value (k, c, idx);
  piece_counts[c][k]++;
  if (k == PAWN) pawn_counts[c][idx_to_file (idx)]++;

  // Update occupancy sets.
  occupied |= masks_0[idx];
  occupied_45 |= masks_45[idx];
  occupied_90 |= masks_90[idx];
  occupied_135 |= masks_135[idx];
}

// Set a piece on the board with a row and file.
void
Board::set_piece (Kind k, Color c, int rank, int file) {
  set_piece (k, c, to_idx (rank, file));
}

//////////////////////
// Move application //
//////////////////////

// Apply a move to the board. Return false if this move is illegal
// because it places or leaves the color to move in check.
bool
Board::apply (const Move &m) {
  Undo dummy;
  return apply (m, dummy);
}

bool
Board::apply (const Move &m, Undo &u) {
  const Kind kind = m.get_kind ();
  const Kind capture = m.get_capture ();
  const Color color = to_move ();

  assert (capture != KING);

  ///////////////////////////
  // Save undo information //
  ///////////////////////////

  u.en_passant      = flags.en_passant;
  u.w_has_k_castled = flags.w_has_k_castled;
  u.w_has_q_castled = flags.w_has_q_castled;
  u.w_can_q_castle  = flags.w_can_q_castle;
  u.w_can_k_castle  = flags.w_can_k_castle;
  u.b_has_k_castled = flags.b_has_k_castled;
  u.b_has_q_castled = flags.b_has_q_castled;
  u.b_can_q_castle  = flags.b_can_q_castle;
  u.b_can_k_castle  = flags.b_can_k_castle;
  u.half_move_clock = half_move_clock;

  ///////////////////
  // Update clocks //
  ///////////////////

  // Reset the 50 move rule clock when a pawn is moved or a capture is
  // made.
  if (capture != NULL_KIND)
    {
      half_move_clock = 0;
    }
  else
    {
      half_move_clock++;
    }

  // Increment the whole move clock after each move by black.
  if (color == BLACK) full_move_clock++;

  if (kind == PAWN)
    {
      ////////////////////////////////////////////////
      // Zero the half move clock when a pawn moves //
      ////////////////////////////////////////////////

      half_move_clock = 0;

      /////////////////////////////////////////////
      // Handle the case of capturing En Passant //
      /////////////////////////////////////////////

      if (m.is_en_passant ())
        {
          if (color == WHITE)
            {
              clear_piece (PAWN, BLACK, flags.en_passant - 8);
            }
          else
            {
              clear_piece (PAWN, WHITE, flags.en_passant + 8);
            }
        }

      /////////////////////////////////////////
      // Set the En Passant square correctly //
      /////////////////////////////////////////

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

      if (kind == KING)
        {
          bool is_castle_qs = m.is_castle_qs ();
          bool is_castle_ks = m.is_castle_ks ();

          ////////////////////////////
          // Update castling status //
          ////////////////////////////

          if (color == WHITE)
            {
              set_castling_right (W_QUEEN_SIDE, false);
              set_castling_right (W_KING_SIDE, false);
            }
          else
            {
              set_castling_right (B_QUEEN_SIDE, false);
              set_castling_right (B_KING_SIDE, false);
            }

          if (is_castle_qs || is_castle_ks)
            {
              ///////////////////////////
              // Handle castling moves //
              ///////////////////////////

              const bitboard attacked = attack_set (invert (color));

              if (color == WHITE)
                {
                  if (is_castle_qs)
                    {
                      clear_piece (KING, WHITE, E1);
                      clear_piece (ROOK, WHITE, A1);
                      set_piece (KING, WHITE, C1);
                      set_piece (ROOK, WHITE, D1);
                      set_color (invert (to_move ()));
                      flags.w_has_q_castled = 1;
                      return !(attacked & 0x1C);
                    }

                  if (is_castle_ks)
                    {
                      clear_piece (KING, WHITE, E1);
                      clear_piece (ROOK, WHITE, H1);
                      set_piece (KING, WHITE, G1);
                      set_piece (ROOK, WHITE, F1);
                      set_color (invert (to_move ()));
                      flags.w_has_k_castled = 1;
                      return !(attacked & 0x70);
                    }
                }
              else
                {
                  byte attacks = get_byte (attacked, 7);

                  if (is_castle_qs)
                    {
                      clear_piece (KING, BLACK, E8);
                      clear_piece (ROOK, BLACK, A8);
                      set_piece (KING, BLACK, C8);
                      set_piece (ROOK, BLACK, D8);
                      set_color (invert (to_move ()));
                      flags.b_has_q_castled = 1;
                      return !(attacks & 0x1C);
                    }

                  if (is_castle_ks)
                    {
                      clear_piece (KING, BLACK, E8);
                      clear_piece (ROOK, BLACK, H8);
                      set_piece (KING, BLACK, G8);
                      set_piece (ROOK, BLACK, F8);
                      set_color (invert (to_move ()));
                      flags.b_has_k_castled = 1;
                      return !(attacks & 0x70);
                    }
                }

              assert (0);
            }
        }
    }

  ////////////////////////////////////////////////////////////////////
  // Adjust castling rights in the case a rook is moved or captured //
  ////////////////////////////////////////////////////////////////////

  if (kind == ROOK || capture == ROOK)
    {
      if (m.from == A1 || m.to == A1)
        set_castling_right (W_QUEEN_SIDE, false);
      if (m.from == H1 || m.to == H1)
        set_castling_right (W_KING_SIDE, false);
      if (m.from == A8 || m.to == A8)
        set_castling_right (B_QUEEN_SIDE, false);
      if (m.from == H8 || m.to == H8)
        set_castling_right (B_KING_SIDE, false);
    }

  //////////////////////////
  // Update color to move //
  //////////////////////////

  set_color (~to_move ());

  //////////////////////////////////////////////
  // Clear the origin and destination squares //
  //////////////////////////////////////////////

  clear_piece (kind, color, m.from);
  if (capture != NULL_KIND) 
    clear_piece (capture, ~color, m.to);
  
  ////////////////////////////////////////////////////
  // Set the destination square, possibly promoting //
  ////////////////////////////////////////////////////
  
  Kind destination_kind = kind;
  if (m.promote != NULL_KIND) destination_kind = m.promote;
    
  set_piece (destination_kind, color, m.to);

  /////////////////////////////////////////////
  // Test legality of the resulting position //
  /////////////////////////////////////////////

  return !in_check (color);
}

void
Board::unapply (const Move &m, const Undo &u) {

  ///////////////////////////////////
  // Restore information from undo //
  ///////////////////////////////////

  flags.en_passant      = u.en_passant;      
  flags.w_has_k_castled = u.w_has_k_castled; 
  flags.w_has_q_castled = u.w_has_q_castled; 
  flags.w_can_q_castle  = u.w_can_q_castle;  
  flags.w_can_k_castle  = u.w_can_k_castle;  
  flags.b_has_k_castled = u.b_has_k_castled; 
  flags.b_has_q_castled = u.b_has_q_castled; 
  flags.b_can_q_castle  = u.b_can_q_castle;  
  flags.b_can_k_castle  = u.b_can_k_castle;  
  half_move_clock       = u.half_move_clock; 

  // Decrement the whole move clock if black moved last.
  if (m.get_color () == WHITE && full_move_clock > 1) full_move_clock--;
  
  if (m.get_kind () == PAWN)
    {
      // Undo en passant.
      if (m.is_en_passant ())
        {
          if (m.get_color () == WHITE)
            {
              set_piece (PAWN, BLACK, flags.en_passant - 8);
            }
          else
            {
              set_piece (PAWN, WHITE, flags.en_passant + 8);
            }
        }
    }

  else if (m.get_kind () == KING)
    {
      bool is_castle_qs = m.is_castle_qs ();
      bool is_castle_ks = m.is_castle_ks ();

      ////////////////////
      // Undo castling. //
      ////////////////////
      
      if (is_castle_qs || is_castle_ks)
        {
          if (m.get_color () == WHITE)
            {
              if (is_castle_qs)
                {
                  set_piece (KING, WHITE, E1);
                  set_piece (ROOK, WHITE, A1);
                  clear_piece (KING, WHITE, C1);
                  clear_piece (ROOK, WHITE, D1);
                  set_color (invert (to_move ()));
                  return;
                }
              if (is_castle_ks)
                {
                  set_piece (KING, WHITE, E1);
                  set_piece (ROOK, WHITE, H1);
                  clear_piece (KING, WHITE, G1);
                  clear_piece (ROOK, WHITE, F1);
                  set_color (invert (to_move ()));
                  return;
                }
            }
          else
            {
              if (is_castle_qs)
                {
                  set_piece (KING, BLACK, E8);
                  set_piece (ROOK, BLACK, A8);
                  clear_piece (KING, BLACK, C8);
                  clear_piece (ROOK, BLACK, D8);
                  set_color (invert (to_move ()));
                  return;
                }
              if (is_castle_ks)
                {
                  set_piece (KING, BLACK, E8);
                  set_piece (ROOK, BLACK, H8);
                  clear_piece (KING, BLACK, G8);
                  clear_piece (ROOK, BLACK, F8);
                  set_color (invert (to_move ()));
                  return;
                }
            }
        }
    }

  //////////////////////////.
  // Restore color to move //
  ///////////////////////////

  set_color (invert (to_move ()));

  ////////////////////////////////////
  // Restore the destination square //
  ////////////////////////////////////

  if (m.is_promote ())
    {
      clear_piece (m.get_promote (), m.get_color (), m.to);
    }
  else
    {
      clear_piece (m.get_kind (), m.get_color (), m.to);
    }

  if (m.is_capture () && !m.is_en_passant ())
    {
      set_piece (m.capture, ~m.get_color (), m.to);
    }
  
  ///////////////////////////////
  // Restore the origin square //
  ///////////////////////////////
  
  set_piece (m.get_kind (), m.get_color (), m.from);
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
            << (Coord) (m.from) << " => " << (Coord) (m.to) << " "
            << m.color << " " << m.kind << " " << m.capture << " "
            << m.promote << " " << m.en_passant << "]";
}

/////////////
// Testing //
/////////////

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
  if (to_move () == WHITE) 
    h ^= zobrist_key_white_to_move;

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
          h ^= get_zobrist_piece_key (c, k, i);
        }
    }

  return h;
}
