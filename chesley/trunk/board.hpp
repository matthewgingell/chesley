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

#include "bits64.hpp"

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

inline Color 
invert_color (Color c) {
  return (Color) -c;
}

/*****************/
/*  Status type  */
/*****************/

enum Status { GAME_IN_PROGRESS = 0, GAME_WIN_WHITE, GAME_WIN_BLACK, GAME_DRAW };

std::ostream & operator<< (std::ostream &os, Status s);

/******************/
/*  Piece kinds.  */
/******************/

enum Kind { NULL_KIND = -1, PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING };

// Convert a kind to a character code.
char to_char (Kind k);

std::ostream & operator<< (std::ostream &os, Kind k);

/**************/
/* Move type. */
/**************/

struct Move {
  Move () {}

  Move (Kind k, uint32 f, uint32 t, Color c, Kind capture) :
    kind (k), color (c), from (f), to (t)
  {
    score = 0;
    flags.capture = capture;
    flags.promote = (Kind) 0;
    flags.castle_qs = 0;
    flags.castle_ks = 0;
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

  uint32 score;      // Score for this move.

  // Return a description of this move in coordinate algebraic
  // notation.
  std::string to_calg () const;
};

// Estimate the value of this move as zero or the value of the piece
// being captured.
inline int
value (const Move &m) {
  switch (m.flags.capture) 
    {
    case NULL_KIND: return 0;
    case PAWN:   return -1;
    case ROOK:   return -5;
    case KNIGHT: return -3;
    case BISHOP: return -3;
    case QUEEN:  return -9;
    default:     assert (0);
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

  /**********************/
  /* Precomputed tables */
  /**********************/

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
    Status   status          :2; // GAME_IN_PROGRESS, etc.
  } flags;
  
  // Clocks
  uint32 half_move_clock; // Used for 50-move rule.
  uint32 full_move_clock; // Clock after each black move.

  int score;

  /************************************/
  /* Constructors and initialization. */
  /************************************/

  Board () {}

  // Common initialization.
  static void common_init (Board &);

  // Construct from an ascii board representation.
  static Board from_ascii  (const std::string &str);

  // Construct from a fen string.
  static Board from_fen (const std::string &str);

  // Construct a board from the standard starting position.
  static Board startpos ();

  /*********/
  /* Tests */
  /*********/

  // Compute a bitboard of every square color is attacking.
  bitboard attack_set (Color) const;

  // Return the kind of piece on a square.
  Color get_color (uint32 idx) {
    if (test_bit (white, idx)) return WHITE;
    if (test_bit (black, idx)) return BLACK;
    return NULL_COLOR;
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

  // Test whether a coordinate is in bounds.
  static bool 
  in_bounds (int x, int y) {
    return x >= 0 && x <= 7 && y >= 0 && y <= 7;
  }
  
  /*************/
  /* Accessors */
  /*************/

  // Return a board reference from a Kind.
  bitboard &
  kind_to_board (Kind k) {
    switch (k) 
      {
      case NULL_KIND: assert (0);
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

  // Clear a piece on the board.
  void 
  clear_piece (int at) {
    if (occupied & masks_0[at])
      {
	white        &= ~masks_0[at];
	black        &= ~masks_0[at];
	pawns        &= ~masks_0[at];
	rooks        &= ~masks_0[at];
	knights      &= ~masks_0[at];
	bishops      &= ~masks_0[at];
	queens       &= ~masks_0[at];
	kings        &= ~masks_0[at];
	occupied     &= ~masks_0[at];
	occupied_45  &= ~masks_45[at];
	occupied_90  &= ~masks_90[at];
	occupied_135 &= ~masks_135[at];
      }
  }

  // Set a piece on the board with an index.
  void 
  set_piece (Kind kind, Color color, int at) {
    color_to_board (color) |= masks_0[at];
    kind_to_board (kind) |= masks_0[at];
    occupied |= masks_0[at];
    occupied_45 |= masks_45[at];
    occupied_90 |= masks_90[at];
    occupied_135 |= masks_135[at];
  }

  // Set a piece on the board with a row and file.
  void
  set_piece (Kind kind, Color color, int row, int file) {
    set_piece (kind, color, file + 8 * row);
  }

  // Apply a move to this board.
  bool apply (const Move &m);

  // Test whether color is in check at this position.
  bool 
  in_check (Color c) const
  {
    // Generate the attack set for the other color.
    bitboard attacked = attack_set (invert_color (c));

    // Find our king.
    bitboard king = kings & (c == WHITE ? white : black);

    // Return whether it's under attack.
    return king & attacked;
  }

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

  /*********/
  /* Debug */
  /*********/

  // Generate the number of moves available at ply 'd'. Used for
  // debugging the move generator.
  uint64 perft (int d) const;

  // Print the full tree to depth N.
  void print_tree (int depth = 0);

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
