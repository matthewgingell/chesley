////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// board.hpp                                                                  //
//                                                                            //
// Representation and operations on a board state in a game of chess.         //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _BOARD_
#define _BOARD_

#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

#include "bits64.hpp"
#include "types.hpp"
#include "util.hpp"

////////////
// Types. //
////////////

struct Move;
struct Move_Vector;
struct Board;

////////////////////
// Bitboard type. //
////////////////////

typedef uint64 bitboard;

void print_board (bitboard b);

////////////////
// Move type. //
////////////////

struct Move {

  // Constructors.

  // It's important we have a null constructor here as we create large
  // arrays of moves which we never access.
  Move () {}

  // Construct a move;
  Move (uint8 from, uint8 to, Kind promote = NULL_KIND, Score score = 0) 
    :from (from), to (to), promote (promote), score (score) {
  };

  // Construct a null move with a score. 
  explicit Move (Score score) 
    :from (0), to (0), promote (NULL_KIND), score (score) {
  };

  // Is this a null move?
  inline bool is_null () const;

  // Kind of color of the piece being moved.
  inline Color get_color (const Board &b) const;

  // Get the kind of the piece being moved.
  inline Kind get_kind (const Board &b) const; 

  // Returns the piece being captured, or NULL_KIND if this is not a
  // capture.
  inline Kind capture (const Board &b) const;

  // Is this move an en passant capture.
  inline bool is_en_passant (const Board &b) const;

  // Is this a castling move?
  inline bool is_castle (const Board &b) const;

  // Is this a queen-side castle.
  inline bool is_castle_qs (const Board &b) const;

  // Is this a king-side castle.
  inline bool is_castle_ks (const Board &b) const;

  // State
  int8 from, to;
  Kind promote :16;
  Score score;
};

inline bool 
less_than (const Move &lhs, const Move &rhs) {
  return lhs.score < rhs.score;
}

inline bool 
more_than (const Move &lhs, const Move &rhs) {
  return lhs.score > rhs.score;
}

///////////////////
// Move vectors. //
///////////////////

// Vector of moves.
struct Move_Vector {

  // Amazingly, examples of positions with 218 different possible
  // moves exist.
  static int const SIZE = 256;

  Move_Vector ();
  Move_Vector (const Move &m);
  Move_Vector (const Move_Vector &mv);
  Move_Vector (const Move &m, const Move_Vector &mv);
  Move_Vector (const Move_Vector &mv, const Move &m);
  Move_Vector (const Move_Vector &mvl, const Move_Vector &mvr);
  Move_Vector (const Board &b);

  void clear () {
    count = 0;
  }

  void push (const Move &m) {
    move[count++] = m;
  }

  void push 
  (uint8 from, uint8 to, Kind promote = NULL_KIND, Score score = 0) {
    move[count++] = Move (from, to, promote, score);
  }

  Move &operator[] (byte i) {
    assert (i < count);
    return move[i];
  }

  Move operator[] (byte i) const {
    assert (i < count);
    return move[i];
  }

  Move move[SIZE];
  byte count;
};

inline int
count (const Move_Vector &moves) {
  return moves.count;
};

std::ostream &
operator<< (std::ostream &os, const Move_Vector &moves);

//////////////////////
// Castling rights. //
//////////////////////

enum Castling_Right {
  W_QUEEN_SIDE, W_KING_SIDE, B_QUEEN_SIDE, B_KING_SIDE
};

/////////////////////////////
// Chess board state type. //
/////////////////////////////

std::ostream & operator<< (std::ostream &os, const Board &b);

struct Board {

  ///////////////
  // Constants //
  ///////////////

  static const std::string INITIAL_POSITIONS;

  static const bitboard light_squares = 0x55AA55AA55AA55AAllu;
  static const bitboard dark_squares = 0xAA55AA55AA55AA55llu;

  //////////////////////////////////////
  // Precomputed tables and constants //
  //////////////////////////////////////

  static bool have_precomputed_tables;

  static bitboard *KNIGHT_ATTACKS_TBL;
  static bitboard *KING_ATTACKS_TBL;
  static bitboard *RANK_ATTACKS_TBL;
  static bitboard *FILE_ATTACKS_TBL;
  static bitboard *DIAG_45_ATTACKS_TBL;
  static bitboard *DIAG_135_ATTACKS_TBL;

  static bitboard *masks_0;
  static bitboard *masks_45;
  static bitboard *masks_90;
  static bitboard *masks_135;

  static int *rot_45;
  static int *rot_90;
  static int *rot_135;

  static byte *diag_shifts_45;
  static byte *diag_bitpos_45;
  static byte *diag_widths_45;
  static byte *diag_shifts_135;
  static byte *diag_bitpos_135;
  static byte *diag_widths_135;

  static uint64 *zobrist_piece_keys;
  static uint64 *zobrist_enpassant_keys;
  static uint64  zobrist_key_white_to_move;
  static uint64  zobrist_w_castle_q_key;
  static uint64  zobrist_w_castle_k_key;
  static uint64  zobrist_b_castle_q_key;
  static uint64  zobrist_b_castle_k_key;

  /////////////////////////////////////////////////////
  // Bitboards representing the state of the board.  //
  /////////////////////////////////////////////////////

  // Color
  bitboard white;
  bitboard black;

  // Pieces
  bitboard pawns;
  bitboard rooks;
  bitboard knights;
  bitboard bishops;
  bitboard queens;
  bitboard kings;

  // Rotated bitboards, for file and diagonal attacks.
  bitboard occupied;
  bitboard occupied_45;
  bitboard occupied_90;
  bitboard occupied_135;

  // Other status flags.

  struct {
    Color    to_move         :8; // BLACK or WHITE.
    unsigned en_passant      :8; // En Passant target square.
    unsigned w_has_k_castled :1; // White has castled king side.
    unsigned w_has_q_castled :1; // White has castled queen side.
    unsigned w_can_q_castle  :1; // White king and q-side rook unmoved.
    unsigned w_can_k_castle  :1; // White king and k-side rook unmoved.
    unsigned b_has_k_castled :1; // Black has castled king side.
    unsigned b_has_q_castled :1; // Black has castled queen side.
    unsigned b_can_q_castle  :1; // Black king and q-side rook unmoved.
    unsigned b_can_k_castle  :1; // Black king and k-side rook unmoved.
  } flags;

  // Clocks
  uint32 half_move_clock; // Used for 50-move rule.
  uint32 full_move_clock; // Clock after each black move.

  //////////////////////////////////////
  // Constructors and initialization. //
  //////////////////////////////////////

  Board () {}

  // Must be called to initialize static members in correct order.
  static const void precompute_tables ();

  // Common initialization.
  static void common_init (Board &);

  // Construct from an ascii board representation.
  static Board from_ascii  (const std::string &str);

  // Construct from a FEN or EPD string.
  static Board from_fen (const std::string &fen, bool EPD = false);
  static Board from_fen (const string_vector &toks, bool EPD = false);

  // Construct a board from the standard starting position.
  static Board startpos ();

  ///////////////
  // Operators //
  ///////////////

  bool operator== (const Board &rhs) {
    return hash == rhs.hash;
  }

  bool operator!= (const Board &rhs) {
    return hash != rhs.hash;
  }

  /////////////
  // Hashing //
  /////////////

  // Incrementally updated hash key for this position.
  uint64 hash;

  // Fetch the key for a piece.
  static uint64 get_zobrist_piece_key (Color c, Kind k, int32 idx) {
    uint32 i = (c == BLACK ? 0 : 1);
    uint32 j = (int32) k;
    assert (c != NULL_COLOR);
    assert (k != NULL_KIND);
    assert (idx < 64);
    assert ((i * (64 * 6) + j * (64) + idx) < 2 * 6 * 64);
    return zobrist_piece_keys[i * (64 * 6) + j * (64) + idx];
  }

  ////////////
  // Output //
  ////////////

  // Return an ASCII representation of this position.
  std::string to_ascii () const;

  // Return a FEN string for this position.
  std::string to_fen () const;

  ///////////////////////////////////////////////////
  // Reading and writing Moves against this board. //
  ///////////////////////////////////////////////////

  // Produce an algebraic letter-number pair for an integer square
  // number.
  std::string to_alg_coord (int idx) const;

  // Produce an an integer square number from an algebraic letter-number
  // pair.
  int from_alg_coord (const std::string &s) const;

  // Does this string look like a move written in coordinate algebraic
  // notation?
  bool is_calg (const std::string &s) const;

  // Construct a Move from coordinate algebraic notation.
  Move from_calg (const std::string &s) const;

  // Return a description of a Move in coordinate algebraic notation.
  std::string to_calg (const Move &m) const;

  // Produce a SAN string from a move.
  std::string to_san (const Move &m) const;

  // Produce a move from a SAN string.
  Move from_san (const std::string &s) const;

  ///////////
  // Tests //
  ///////////

  // Compute a bitboard of every square color is attacking.
  bitboard attack_set (Color) const;

  // Get the number of legal moves available from this position.
  int child_count () const;

  // Return whether the square idx is attacked by a piece of color c.
  bool is_attacked (int idx, Color c) const;

  // For the currently moving side, find the least valuable piece
  // attacking a square.
  Move least_valuable_attacker (int sqr) const;

  // Return whether color c is in check.
  bool in_check (Color c) const;

  // Return the color to move.
  Color to_move () const {
    return flags.to_move;
  }
  
  // Return the color of a piece on a square.
  Color get_color (uint32 idx) const {
    if (test_bit (white, idx)) return WHITE;
    if (test_bit (black, idx)) return BLACK;
    return NULL_COLOR;
  }

  // Return the color of a piece on a square.
  Color get_color (int row, int file) const {
    return get_color (file + 8 * row);
  }

  // Return the kind of piece on a square.
  Kind get_kind (uint32 idx) const {
    if (!test_bit (occupied, idx)) return NULL_KIND;
    if ( test_bit    (pawns, idx)) return PAWN;
    if ( test_bit    (rooks, idx)) return ROOK;
    if ( test_bit  (knights, idx)) return KNIGHT;
    if ( test_bit  (bishops, idx)) return BISHOP;
    if ( test_bit   (queens, idx)) return QUEEN;
    if ( test_bit    (kings, idx)) return KING;

    assert (0);

    // Suppress gcc warning in -DNDEBUG case.
    return NULL_KIND;
  }

  // Return the kind of piece on a square.
  Kind get_kind (int row, int file) const {
    return get_kind (file + 8 * row);
  }

  // Test whether a coordinate is in bounds.
  static bool
  in_bounds (int x, int y) {
    return x >= 0 && x <= 7 && y >= 0 && y <= 7;
  }

  // Get a bitboard of pieces of some color.
  bitboard get_pawns   (Color c) { return color_to_board (c) & pawns; }
  bitboard get_rooks   (Color c) { return color_to_board (c) & rooks; }
  bitboard get_knights (Color c) { return color_to_board (c) & knights; }
  bitboard get_bishops (Color c) { return color_to_board (c) & bishops; }
  bitboard get_queens  (Color c) { return color_to_board (c) & queens; }
  bitboard get_kings   (Color c) { return color_to_board (c) & kings; }

  ///////////////////
  // Flags setters //
  ///////////////////

  // It is crucial to use the following routines to set board flags,
  // rather than accessing those fields directly. Otherwise the
  // incrementally maintained hash key will not be properly updated.

  // Set color.
  void set_color (Color c) {
    // If the color is changing, we need to flip the hash bits for
    // zobrist_key_white_to_move.
    assert (c != NULL_COLOR);
    if (flags.to_move != c)
      hash ^= Board::zobrist_key_white_to_move;
    flags.to_move = c;
  }

  // Set en_passant target.
  void set_en_passant (int32 idx) {
    hash ^= zobrist_enpassant_keys[flags.en_passant];
    hash ^= zobrist_enpassant_keys[idx];
    flags.en_passant = idx;
  }

  // Set castling rights.
  void set_castling_right (Castling_Right cr, bool v);

  ///////////////
  // Accessors //
  ///////////////

  // Return a board reference from a Kind.
  bitboard &
  kind_to_board (Kind k) {
    switch (k)
      {
      case NULL_KIND: assert (0); break;
      case PAWN: return pawns; break;
      case ROOK: return rooks; break;
      case KNIGHT: return knights; break;
      case BISHOP: return bishops; break;
      case KING: return kings; break;
      case QUEEN: return queens; break;
      default: assert (0);
      }

    // Suppress gcc warning in -DNDEBUG case.
    return pawns;
  }

  // Const version returns by copy.
  bitboard
  kind_to_board (Kind k) const {
    switch (k)
      {
      case NULL_KIND: assert (0); break;
      case PAWN: return pawns; break;
      case ROOK: return rooks; break;
      case KNIGHT: return knights; break;
      case BISHOP: return bishops; break;
      case KING: return kings; break;
      case QUEEN: return queens; break;
      default: assert (0);
      }

    // Suppress gcc warning in -DNDEBUG case.
    return pawns;
  }


  // Map a color to the corresponding bitboard.
  bitboard &
  color_to_board (Color color) {
    switch (color)
      {
      case WHITE: return white; break;
      case BLACK: return black; break;
      default: assert (0);
      }

    // Suppress gcc warning in -DNDEBUG case.
    return white;
  }

  // Const version returns by copy.
  bitboard
  color_to_board (Color color) const {
    switch (color)
      {
      case WHITE: return white; break;
      case BLACK: return black; break;
      default: assert (0);
      }

    // Suppress gcc warning in -DNDEBUG case.
    return white;
  }

  // Clear a piece on the board.
  void
  clear_piece (int idx) {
    if (occupied & masks_0[idx])
      {
        Color c = get_color (idx);
        Kind k = get_kind (idx);
        clear_piece (idx, c, k);
      }
  }

  void
  clear_piece (int idx, Color c, Kind k) {
    if (occupied & masks_0[idx])
      {
        assert (k != NULL_KIND);
        assert (c == BLACK || c == WHITE);
        assert (idx >= 0 && idx < 64);
        
        // Clear color and piece kind bits.
        color_to_board (c) &= ~masks_0[idx];
        kind_to_board (k) &= ~masks_0[idx];

        // Update hash key.
        hash ^= get_zobrist_piece_key (c, k, idx);

        // Clear the occupancy sets.
        occupied     &= ~masks_0[idx];
        occupied_45  &= ~masks_45[idx];
        occupied_90  &= ~masks_90[idx];
        occupied_135 &= ~masks_135[idx];
      }
  }

  // Set a piece on the board with an index.
  void
  set_piece (Kind k, Color c, uint32 idx) {
    assert (k != NULL_KIND);
    assert (c == BLACK || c == WHITE);
    assert (idx >= 0 && idx < 64);

    // Update color and piece sets.
    color_to_board (c) |= masks_0[idx];
    kind_to_board (k) |= masks_0[idx];

    // Update hash key.
    hash ^= get_zobrist_piece_key (c, k, idx);

    // Update occupancy sets.
    occupied |= masks_0[idx];
    occupied_45 |= masks_45[idx];
    occupied_90 |= masks_90[idx];
    occupied_135 |= masks_135[idx];
  }

  // Set a piece on the board with a row and file.
  void
  set_piece (Kind kind, Color color, int row, int file) {
    set_piece (kind, color, file + 8 * row);
  }

  // Apply a move to this board.
  bool apply (const Move &m);

  ////////////
  // Boards //
  ////////////

  // Return a bitboard with every bit of the Nth rank set.
  static bitboard
  rank_mask (int rank) {
    return 0x00000000000000FFllu << rank * 8;
  }

  // Return a bitboard with every bit of the Nth file set.
  static bitboard
  file_mask (int file) {
    return 0x0101010101010101llu << file;
  }

  // Return the rank 0 .. 7 containing a piece.
  static int
  idx_to_rank (int idx) {
    return idx / 8;
  }

  // Return the file 0 .. 7 containing a piece.
  static int
  idx_to_file (int idx) {
    return idx % 8;
  }

  // Return a bitboard of to_moves pieces.
  bitboard
  our_pieces () const {
    return to_move () == WHITE ? white : black;
  }

  // Return a bitboard of the other colors pieces.
  bitboard
  other_pieces () const {
    return to_move () == WHITE ? black : white;
  }

  // Return a bitboard of unoccupied squares.
  bitboard
  unoccupied () const {
    return ~occupied;
  }

  /////////////////////////
  // Occupancy patterns. //
  /////////////////////////

  byte
  occ_0 (int from) const {
    return get_byte (occupied, from / 8);
  }

  byte
  occ_45 (int from) const {
    return occupied_45 >> diag_shifts_45[from];
  }

  byte
  occ_90 (int from) const {
    return get_byte (occupied_90, from % 8);
  }

  byte
  occ_135 (int from) const {
    return occupied_135 >> diag_shifts_135[from];
  }

  /////////////////////////////////////////
  // Move and child position generation. //
  /////////////////////////////////////////

  void gen_moves (Move_Vector &moves) const;
  void gen_captures (Move_Vector &moves) const;

  //////////////
  // Testing. //
  //////////////

  // Generate the number of moves available at ply 'd'. Used for
  // debugging the move generator.
  uint64 perft (int d) const;

  // For each child, print the child move and the perft (d) of the
  // resulting board.
  void divide (int d) const;

  // Print the full tree to depth N.
  void print_tree (int depth = 0);

  // Generate a hash key from scratch. This is used to test the
  // correctness of our incremently hash update code.
  uint64 gen_hash () const;
};

// Output human readable board.
std::ostream & operator<< (std::ostream &os, const Board &b);

///////////
// Moves //
///////////

// Is this a null move?
inline bool
Move::is_null () const { 
  return to == from;
}

// Kind of color of the piece being moved.
inline Color 
Move::get_color (const Board &b) const {
  return b.get_color (from);
}

// Get the kind of the piece being moved.
inline Kind 
Move::get_kind (const Board &b) const {
  return b.get_kind (from);
}

// Returns the piece being captured, or NULL_KIND if these is no
// such piece.
inline Kind 
Move::capture (const Board &b) const {
  if (is_en_passant (b))
    {
      return PAWN;
    }
  else
    {
      return b.get_kind (to);
    }
}

// Is this move an en passant capture.
inline bool 
Move::is_en_passant (const Board &b) const {
  // If the piece being moved is a pawn, the square being moved to is
  // empty, and the to square and the from sqare are on different
  // files, then this is an enpassant capture.
  return get_kind (b) == PAWN
    && ((test_bit (b.occupied, to) == 0))
    && (Board::idx_to_file (to) != Board::idx_to_file (from));
}

// Is this a castling move?
inline bool
Move::is_castle (const Board &b) const {
  return is_castle_qs (b) || is_castle_ks (b);
}

// Is this a queen-side castle.
inline bool 
Move::is_castle_qs (const Board &b) const {
  if ((b.to_move () == WHITE && from == E1 && to == C1) || 
      (b.to_move () == BLACK && from == E8 && to == C8)) 
    {
      return (b.get_kind (from) == KING);
    }
  else
    {
      return false;
    }
}

// Is this a king-side castle.
inline bool 
Move::is_castle_ks (const Board &b) const {
  if ((b.to_move () == WHITE && from == E1 && to == G1) ||
      (b.to_move () == BLACK && from == E8 && to == G8))
    {
      return (b.get_kind (from) == KING);
    }
  else
    {
      return false;
    }
}

// Equality on moves.
inline bool operator== (const Move &lhs, const Move &rhs) {
  return 
    (lhs.from == rhs.from) && 
    (lhs.to == rhs.to) && 
    (lhs.promote == rhs.promote);
}

inline bool operator!= (const Move &lhs, const Move &rhs) {
  return 
    (lhs.from != rhs.from) || 
    (lhs.to != rhs.to) ||
    (lhs.promote != rhs.promote);
}

// Output for moves.
std::ostream & operator<< (std::ostream &os, const Move &b);

#endif // _BOARD_
