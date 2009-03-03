/*
  Representation and operations on a board state in a game of chess.

  Matthew Gingell
  gingell@adacore.com
*/

#ifndef _BOARD_
#define _BOARD_

#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

#include "util.hpp"

/*********/
/* Types */
/*********/

struct Move;
struct Move_Vector;
struct Board;
struct Board_Vector;

/*******************/
/*  Bitboard type. */
/*******************/

typedef bits64 bitboard;

void print_board (bitboard b);
void print_masks (bitboard b);

/*****************/
/*  Color type.  */
/*****************/

enum Color { WHITE = 1, NULL_COLOR = 0, BLACK = -1 };

std::ostream & operator<< (std::ostream &, Color);

inline Color invert_color (Color c) { return (Color) -c; }

/*****************/
/*  Status type  */
/*****************/

enum Status { GAME_IN_PROGRESS = 0, GAME_WIN_WHITE, GAME_WIN_BLACK, GAME_DRAW };

std::ostream & operator<< (std::ostream &os, Status s);

/******************/
/*  Piece kinds.  */
/******************/

// This type is used to index tables, so PAWN must always be set to
// zero, etc.
enum Kind { NULL_KIND = -1, PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING };

// Convert a kind to a character code.
char to_char (Kind k);

// Convert a character code to a piece code, ignoring color.
Kind to_kind (char k);

std::ostream & operator<< (std::ostream &os, Kind k);

/**************/
/* Move type. */
/**************/

struct Move {
  Move () {}

  Move (Kind k, uint32 f, uint32 t, Color c, Kind capture, int32 score = 0) :
    kind (k), color (c), from (f), to (t), score (score) {
    score = 0;
    flags.capture = capture;
    flags.promote = (Kind) 0;
    flags.castle_qs = 0;
    flags.castle_ks = 0;
  }

  // Create a null move with a score.
  Move (int32 score) : score (score) {
    from = to = 0;
    kind = NULL_KIND;
    color = NULL_COLOR;
    flags.capture = NULL_KIND;
    flags.promote = NULL_KIND;
    flags.castle_qs = flags.castle_ks = 0;
  }

  struct {
    Kind     capture   : 4; // Kind being captured.
    Kind     promote   : 4; // Kind being promoted to.
    unsigned castle_qs : 1; // Is this a qs castle?
    unsigned castle_ks : 1; // Is this a ks castle?
  } flags;

  Kind   kind   :4;  // Kind being moved.
  Color  color  :3;  // Color being moved.

  uint32 from   :6;  // Origin.
  uint32 to     :6;  // Destination

  int32  score;      // Score for this move.
};

inline Move operator- (Move m) {
  m.score = -m.score;
  return m;
}

inline Move max (Move l, Move r) {
  return ((l.score > r.score) ? l : r);
}

inline Move min (Move l, Move r) {
  return ((l.score < r.score) ? l : r);
}

// Estimate the value of this move as zero or the value of the piece
// being captured.
inline int
value (const Move &m) {
  switch (m.flags.capture)
    {
    case NULL_KIND: return  0;
    case PAWN:      return -1;
    case ROOK:      return -5;
    case KNIGHT:    return -3;
    case BISHOP:    return -3;
    case QUEEN:     return -9;
      //    default:     assert (0);
    }

  // Suppress gcc warning in -DNDEBUG case.
  return 0;
}

std::ostream & operator<< (std::ostream &, const Move &);

/****************/
/* Move vectors */
/****************/

// Vector of moves.
struct Move_Vector {

  // Amazingly, examples of positions with 218 different possible
  // moves exist.
  static int const SIZE = 256;

  Move_Vector () {
    count = 0;
  }

  Move_Vector (const Board &b);

  void push (const Move &m) {
    move[count++] = m;
  }

  Move &operator[] (int i) {
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

/*******************/
/* Castling rights */
/*******************/

enum Castling_Right
  {W_QUEEN_SIDE, W_KING_SIDE, B_QUEEN_SIDE, B_KING_SIDE};

/****************************/
/* Chess board state type. */
/***************************/

std::ostream & operator<< (std::ostream &, const Board &);

struct Board {

  // Must be called to initialize static members in correct order.
  static const void precompute_tables ();

  /*************/
  /* Constants */
  /*************/

  static const std::string INITIAL_POSITIONS;

  /************************************/
  /* Precomputed tables and constants */
  /************************************/

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

  /***************************************************/
  /* Bitboards representing the state of the board.  */
  /***************************************************/

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
    Color    to_move         :2; // BLACK or WHITE.
    unsigned w_has_k_castled :1; // White has castled king side.
    unsigned w_has_q_castled :1; // White has castled queen side.
    unsigned w_can_q_castle  :1; // White king and q-side rook unmoved.
    unsigned w_can_k_castle  :1; // White king and k-side rook unmoved.
    unsigned b_has_k_castled :1; // Black has castled king side.
    unsigned b_has_q_castled :1; // Black has castled queen side.
    unsigned b_can_q_castle  :1; // Black king and q-side rook unmoved.
    unsigned b_can_k_castle  :1; // Black king and k-side rook unmoved.
    unsigned en_passant      :6; // En Passant target square.
  } flags;

  // Clocks
  uint32 half_move_clock; // Used for 50-move rule.
  uint32 full_move_clock; // Clock after each black move.

  int32 score;

  /************************************/
  /* Constructors and initialization. */
  /************************************/

  Board () {}

  // Common initialization.
  static void common_init (Board &);

  // Construct from an ascii board representation.
  static Board from_ascii  (const std::string &str);

  // Construct from a FEN or EPD string.
  static Board from_fen (const std::string &fen, bool EPD = false);
  static Board from_fen (const string_vector &toks, bool EPD = false);

  // Construct a board from the standard starting position.
  static Board startpos ();

  /***********/
  /* Hashing */
  /***********/

  // Incrementally updated hash key for this position.
  uint64 hash;

  // Fetch the key for a piece.
  static uint64 get_zobrist_piece_key (Color c, Kind k, int32 idx) {
    uint32 i = (c == BLACK ? 0 : 1);
    uint32 j = (int32) k;

    assert (c != NULL_COLOR);
    assert (k != NULL_KIND);
    assert (idx < 64);

    if ((i * (64 * 6) + j * (64) + idx) >= 2 * 6 * 64)
      {
	std::cerr << i * (64 * 6) + j * (64) + idx << std::endl;
      }

    assert ((i * (64 * 6) + j * (64) + idx) < 2 * 6 * 64);

    return zobrist_piece_keys[i * (64 * 6) + j * (64) + idx];
  }

  /**********/
  /* Output */
  /**********/

  // Return an ASCII representation of this position.
  std::string to_ascii () const;

  // Return a FEN string for this position.
  std::string to_fen () const;

  /*************************************************/
  /* Reading and writing Moves against this board. */
  /*************************************************/

  // Does this string look like a move written in coordinate algebraic
  // notation?
  bool is_calg (const std::string &s) const;

  // Construct a Move from coordinate algebraic notation.
  Move from_calg (const std::string &s) const;

  // Return a description of a Move in coordinate algebraic notation.
  std::string to_calg (const Move &m) const;

  /*********/
  /* Tests */
  /*********/

  // Compute a bitboard of every square color is attacking.
  bitboard attack_set (Color) const;

  // Get the number of legal moves available from this position.
  int child_count ();

  // Get the status of the game.
  Status get_status ();

  // Return whether color c is in check.
  bool in_check (Color c) const;

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

  /****************/
  /* Flags setter */
  /****************/

  // It is crucial to use the following routines to set board flags,
  // rather than accessing those fields directly, becase otherwise the
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

  /*************/
  /* Accessors */
  /*************/

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

	assert (c != NULL_COLOR);
	assert (k != NULL_KIND);

	// Clear color and piece kind bits.
	color_to_board (c) &= ~masks_0[idx];
	kind_to_board (k)  &= ~masks_0[idx];

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
    assert (c >= BLACK && c <= WHITE);
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

  /**********/
  /* Boards */
  /**********/

  // Return a bitboard with every bit of the Nth rank set.
  static bitboard
  rank (int rank) {
    return 0x00000000000000FFllu << rank * 8;
  }

  // Return a bitboard with every bit of the Nth file set.
  static bitboard
  file (int file) {
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
    return flags.to_move == WHITE ? white : black;
  }

  // Return a bitboard of the other colors pieces.
  bitboard
  other_pieces () const {
    return flags.to_move == WHITE ? black : white;
  }

  // Return a bitboard of unoccupied squares.
  bitboard
  unoccupied () const {
    return ~occupied;
  }

  /***********************/
  /* Occupancy patterns. */
  /***********************/

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

  /***************************************/
  /* Move and child position generation. */
  /***************************************/

  void gen_all_moves (Move_Vector &moves) const;

  inline void gen_pawn_moves   (Move_Vector &moves) const;
  inline void gen_rook_moves   (Move_Vector &moves) const;
  inline void gen_knight_moves (Move_Vector &moves) const;
  inline void gen_bishop_moves (Move_Vector &moves) const;
  inline void gen_queen_moves  (Move_Vector &moves) const;
  inline void gen_king_moves   (Move_Vector &moves) const;

  /*********************************/
  /* Precomputed table generation. */
  /*********************************/

  // Bit masks for rotated coordinates.
  static bitboard *init_masks_0 ();
  static bitboard *init_masks_45 ();
  static bitboard *init_masks_90 ();
  static bitboard *init_masks_135 ();

  // Maps from normal to rotated coordinates.
  static int *init_rot_45 ();
  static int *init_rot_90 ();
  static int *init_rot_135 ();

  // Assessors for diagonals.
  static byte *init_diag_shifts_45 ();
  static byte *init_diag_bitpos_45 ();
  static byte *init_diag_widths_45 ();
  static byte *init_diag_shifts_135 ();
  static byte *init_diag_bitpos_135 ();
  static byte *init_diag_widths_135 ();

  // Attack tables.
  static bitboard *init_knight_attacks_tbl ();
  static bitboard *init_king_attacks_tbl ();
  static bitboard *init_rank_attacks_tbl ();
  static bitboard *init_file_attacks_tbl ();
  static bitboard *init_45d_attacks_tbl ();
  static bitboard *init_135d_attacks_tbl ();

  // Zobrist keys.
  static void init_zobrist_keys ();

  /***********/
  /* Testing */
  /***********/

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
std::ostream & operator<< (std::ostream &, const Board &);

/*****************/
/* Board vectors */
/*****************/

// Vector of boards.
struct Board_Vector {

  static int const SIZE = 256;

  Board_Vector () {
    count = 0;
  }

  Board_Vector (const Board &b);

  void push (const Board &b) {
    assert (count < SIZE);
    board[count++] = b;
  }

  Board &operator[] (int i) {
    return board[i];
  }

  Board board[SIZE];
  int count;
};

// Count as a function for sorting purposes.
inline int count (const Board_Vector &bv) {
  return bv.count;
}

std::ostream & operator<< (std::ostream &os, Board_Vector bv);

#endif // _BOARD_
