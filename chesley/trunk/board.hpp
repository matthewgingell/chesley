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
#include "move.hpp"
#include "common.hpp"
#include "util.hpp"

////////////
// Types. //
////////////

struct Move_Vector;
struct Board;

void print_board (bitboard b);

/////////////////////////////
// Chess board state type. //
/////////////////////////////

std::ostream & operator<< (std::ostream &os, const Board &b);

struct Board {

  /////////////////////////////////////////////////////
  // Bitboards representing the state of the board.  //
  /////////////////////////////////////////////////////

  // Color.
  bitboard white;
  bitboard black;

  // Pieces.
  bitboard pawns;
  bitboard rooks;
  bitboard knights;
  bitboard bishops;
  bitboard queens;
  bitboard kings;

  // Rotated bitboards for file and diagonal attacks.
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

  // Clocks.
  uint16 half_move_clock; // Used for 50-move rule.
  uint16 full_move_clock; // Clock after each black move.

  // History.
  Move last_move;

  //////////////////////////////////////
  // Constructors and initialization. //
  //////////////////////////////////////

  Board () {}

  // Common initialization.
  static void common_init (Board &);

  // Construct from an ASCII board representation.
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
  uint64 phash;

  // Fetch the key for a piece.
  static uint64 get_zobrist_piece_key (Color c, Kind k, coord idx) {
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
  // Reading and writing moves against this board. //
  ///////////////////////////////////////////////////

  // Produce an algebraic letter-number pair for an integer square
  // number.
  std::string to_alg_coord (coord idx) const;

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
  void from_san_fail (const std::string &) const;

  ///////////
  // Tests //
  ///////////

  // Compute a bitboard of every square color is attacking.
  bitboard attack_set (Color) const;

  // Get the number of legal moves available from this position.
  int child_count () const;

  // Return whether the square idx is attacked by a piece of color c.
  bool is_attacked (coord idx, Color c) const;

  // For the currently moving side, find the least valuable piece
  // attacking a square.
  Move least_valuable_attacker (coord sqr) const;

  // Return whether color c is in check.
  bool in_check (Color c) const;

  // Return the color to move.
  Color to_move () const { return flags.to_move; }

  // Return the color of a piece on a square.
  Color get_color (coord idx) const {
    if (test_bit (white, idx)) return WHITE;
    if (test_bit (black, idx)) return BLACK;
    return NULL_COLOR;
  }

  // Return the color of a piece on a square.
  Color get_color (int row, int file) const {
    return get_color (file + 8 * row);
  }

  // Return the kind of piece on a square.
  Kind get_kind (coord idx) const {
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

  // Get a bitboard of pieces of some color.
  bitboard get_pawns   (Color c) { return color_to_board (c) & pawns; }
  bitboard get_rooks   (Color c) { return color_to_board (c) & rooks; }
  bitboard get_knights (Color c) { return color_to_board (c) & knights; }
  bitboard get_bishops (Color c) { return color_to_board (c) & bishops; }
  bitboard get_queens  (Color c) { return color_to_board (c) & queens; }
  bitboard get_kings   (Color c) { return color_to_board (c) & kings; }

  // Get a bitboard of pawn moves, excluding attacks.
  bitboard get_pawn_moves (Color c) const {
    bitboard to;
    switch (c) 
      {
      case WHITE: 
        // Empty square on step forwards, or two empty squares from rank 2.
        to = ((pawns & white) << 8) & unoccupied ();
        to |= ((to & rank_mask (2)) << 8) & unoccupied (); 
        break;

      case BLACK: 
        // Empty square on step forwards, or two empty squares from rank 2.
        to = ((pawns & black) >> 8) & unoccupied ();
        to |= ((to & rank_mask (5)) >> 8) & unoccupied ();
        break;

      default:
        assert (0);
      }

    return to;
  }

  // Get a bitboard of squares attacked by pawns, including empties.
  bitboard get_pawn_attacks (Color c) const {
    bitboard to, from;

    switch (c)
      {
      case WHITE:
        // Pawns to move.
        from = pawns & white;      

        // Capture forward right, forward left.
        to = ((pawns & white & ~file_mask (A)) << 7);
        to |= ((pawns & white & ~file_mask (H)) << 9);

        // En Passant.
        if (flags.en_passant != 0 &&
            ((bit_idx ((from & ~file_mask (A)) << 7) == flags.en_passant) ||
             (bit_idx ((from & ~file_mask (H)) << 9) == flags.en_passant)))
          to |= masks_0[flags.en_passant];

        break;
        
      case BLACK:
        // Pawns to move.
        from = pawns & white;      

        // Capture backward left, backward right, 
        to = ((pawns & black & ~file_mask (H)) >> 7);
        to |= ((pawns & black & ~file_mask (A)) >> 9);

        // En Passant.
        if (flags.en_passant != 0 &&
            ((bit_idx ((from & ~file_mask (H)) >> 7) == flags.en_passant) ||
             (bit_idx ((from & ~file_mask (A)) >> 9) == flags.en_passant)))
          to |= masks_0[flags.en_passant];

        break;

      default:
        assert (0);
      }

    return to;
  }

  ///////////////////
  // Flags setters //
  ///////////////////

  // It is crucial to use the following routines to set board flags
  // rather than accessing those fields directly. Otherwise the
  // incrementally maintained hash key will not be properly updated.

  // Set color.
  void set_color (Color c) {
    // If the color is changing, we need to flip the hash bits for
    // zobrist_key_white_to_move.
    assert (c != NULL_COLOR);
    if (flags.to_move != c)
      {
        hash ^= zobrist_key_white_to_move;
        flags.to_move = c;
      }
  }

  // Set en_passant target.
  void set_en_passant (coord idx) {
    hash ^= zobrist_enpassant_keys[flags.en_passant];
    hash ^= zobrist_enpassant_keys[idx];
    flags.en_passant = idx;
  }

  // Set castling rights.
  void set_castling_right (Castling_Right cr, bool v);

  ///////////////
  // Assessors //
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
  void clear_piece (coord idx);
  void clear_piece (coord idx, Color c, Kind k);

  // Set a piece on the board.
  void set_piece (Kind k, Color c, coord idx);
  void set_piece (Kind kind, Color color, int row, int file);

  // Apply a move to this board.
  bool apply (const Move &m);

  ////////////
  // Boards //
  ////////////

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
  unoccupied () const { return ~occupied; }

  /////////////////////////
  // Occupancy patterns. //
  /////////////////////////

  byte
  occ_0 (coord from) const {
    return get_byte (occupied, from / 8);
  }

  byte
  occ_45 (coord from) const {
    return (byte) (occupied_45 >> diag_shifts_45[from]);
  }

  byte
  occ_90 (coord from) const {
    return get_byte (occupied_90, from % 8);
  }

  byte
  occ_135 (coord from) const {
    return (byte) (occupied_135 >> diag_shifts_135[from]);
  }

  /////////////////////////////////////////
  // Move and child position generation. //
  /////////////////////////////////////////

  bitboard rank_attacks (coord idx) const { 
    return RANK_ATTACKS_TBL[idx * 256 + occ_0 (idx)];
  }

  bitboard file_attacks (coord idx) const { 
    return FILE_ATTACKS_TBL[idx * 256 + occ_90 (idx)];
  }

  bitboard diag_45_attacks (coord idx) const { 
    return DIAG_45_ATTACKS_TBL[idx * 256 + occ_45 (idx)];
  }

  bitboard diag_135_attacks (coord idx) const { 
    return DIAG_135_ATTACKS_TBL[idx * 256 + occ_135 (idx)];
  }

  bitboard knight_attacks (coord idx) const { 
    return KNIGHT_ATTACKS_TBL[idx];
  }

  bitboard bishop_attacks (coord idx) const { 
    return diag_45_attacks (idx) | diag_135_attacks (idx);
  }

  bitboard rook_attacks (coord idx) const { 
    return rank_attacks (idx) | file_attacks (idx);
  }

  bitboard queen_attacks (coord idx) const { 
    return bishop_attacks (idx) | rook_attacks (idx);
  }

  bitboard king_attacks (coord idx) const { 
    return KING_ATTACKS_TBL[idx];
  }  

  void gen_moves (Move_Vector &moves) const;
  void gen_captures (Move_Vector &moves) const;
  void gen_promotions (Move_Vector &moves) const;

  ////////////////////////
  // Mobility counting. //
  ////////////////////////

  byte rank_mobility (coord idx) const { 
    return RANK_MOBILITY_TBL[idx * 256 + occ_0 (idx)];
  }

  byte file_mobility (coord idx) const { 
    return FILE_MOBILITY_TBL[idx * 256 + occ_90 (idx)];
  }

  byte diag_45_mobility (coord idx) const { 
    return DIAG_45_MOBILITY_TBL[idx * 256 + occ_45 (idx)];
  }

  byte diag_135_mobility (coord idx) const { 
    return DIAG_135_MOBILITY_TBL[idx * 256 + occ_135 (idx)];
  }

  byte knight_mobility (coord idx) const { 
    return KNIGHT_MOBILITY_TBL[idx];
  }

  byte bishop_mobility (coord idx) const { 
    return diag_45_mobility (idx) + diag_135_mobility (idx);
  }

  byte rook_mobility (coord idx) const { 
    return rank_mobility (idx) + file_mobility (idx);
  }

  byte queen_mobility (coord idx) const { 
    return bishop_mobility (idx) + rook_mobility (idx);
  }

  byte king_mobility (coord idx) const { 
    return KING_MOBILITY_TBL[idx];
  }  

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
  // correctness of our incrementally hash update code.
  uint64 gen_hash () const;
  
  ////////////////////////////////////////////////
  // Incrementally updated scoring information. //
  ////////////////////////////////////////////////
  
  Score material[COLOR_COUNT];
  Score psquares[COLOR_COUNT];
  uint8 piece_counts[COLOR_COUNT][KIND_COUNT];
  uint8 pawn_counts[COLOR_COUNT][FILE_COUNT];
};

// Output human readable board.
std::ostream & operator<< (std::ostream &os, const Board &b);

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

#endif // _BOARD_
