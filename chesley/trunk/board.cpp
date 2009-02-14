/*
   Representation and operations on a board state in a game of chess.

   Matthew Gingell
   gingell@adacore.com
*/

#include <cassert>
#include <cctype>
#include <sstream>
#include "chesley.hpp"

using namespace std;

/**********/
/* Debug. */
/**********/

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
    {
      cerr<< *this << endl;
    }
  else
    {
      Board_Vector children (*this);
      for (int i = 0; i < children.count; i++)
	{
	  children[i].print_tree (depth - 1);
	}
    }
}

/*******************/
/*  Status type.  */
/******************/

std::ostream &
operator<< (std::ostream &os, Status s) {
  switch (s)
    {
    case GAME_IN_PROGRESS: os << "GAME_IN_PROGRESS"; break;
    case GAME_WIN_WHITE:   os << "GAME_WIN_WHITE"; break;
    case GAME_WIN_BLACK:   os << "GAME_WIN_BLACK"; break;
    case GAME_DRAW:        os << "GAME_DRAW"; break;
    }
  return os;
}

/**************/
/* Kind type. */
/**************/

// Convert a kind to a character code.
char
to_char (Kind k) {
  switch (k)
    {
    case NULL_KIND: return '?';
    case PAWN:      return 'p';
    case ROOK:      return 'r';
    case KNIGHT:    return 'n';
    case BISHOP:    return 'b';
    case QUEEN:     return 'q';
    case KING:      return 'k';
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

/*****************/
/*  Color type.  */
/*****************/

std::ostream & operator<< (std::ostream &os, Color c) {
  switch (c)
    {
    case WHITE: return os << "WHITE";
    case BLACK: return os << "BLACK";
    default: return os;
    }
}

/*******************************/
/* Move and move vector type.  */
/*******************************/

std::ostream &
operator<< (std::ostream &os, const Move &m)
{
  os << "[MOVE: ";
  os << m.color << " " << m.kind;
  os << " at " << m.from << " => " << m.to;
  if (m.flags.castle_qs) os << " (castling queen side)";
  if (m.flags.castle_ks) os << " (castling king side)";
  if (m.flags.promote) os << " (promoting to)  " << m.flags.promote;
  os << " score: " << m.score ;
  os << "]";
  return os;
}

// Does this string look like a move written in coordinate algebraic
// notation?
bool
Board::is_calg (const string &s) const {
  if (s.length() != 4 ||
      !isalpha (s[0]) || !isdigit (s[1]) ||
      !isalpha (s[2]) || !isdigit (s[3]) )
    {
      return false;
    }
  else
    {
      return true;
    }
}

// Construct a Move from coordinate algebraic notation.
Move 
Board::from_calg (const string &s) const {
  assert (s.length () >= 4);

  // Decode string
  uint32 from = (s[0] - 'a') + 8 * (s[1] - '1');
  uint32 to   = (s[2] - 'a') + 8 * (s[3] - '1');

  // Build move.
  Kind k = get_kind (from);
  Color c = flags.to_move;
  Kind capture = get_kind (to);
  Move m = Move (k, from, to, c, capture);


  // Check for promotion.
  if (s.length () >= 5)
    {
      assert (k == PAWN);
      m.flags.promote = get_kind (s[4]);
    }

  // Set castling flags.
  if (k == KING)
    {
      if (from ==  4 && to ==  2) m.flags.castle_qs = 1;
      if (from ==  4 && to ==  6) m.flags.castle_ks = 1;
      if (from == 60 && to == 58) m.flags.castle_qs = 1;
      if (from == 60 && to == 62) m.flags.castle_ks = 1;
    }

  return m;
}

// Return a description of a Move in coordinate algebraic notation.
string
Board::to_calg (const Move &m) const {
  ostringstream s;

  // The simple case of an ordinary move.
  s << (char) ('a' + idx_to_file (m.from));
  s << (int)  (      idx_to_rank (m.from) + 1);
  s << (char) ('a' + idx_to_file (m.to));
  s << (int)  (      idx_to_rank (m.to) + 1);

  // Promotion case.
  if (m.flags.promote)
    {
      s << to_char (m.flags.promote);
    }

  return s.str();
}

Move_Vector::Move_Vector (const Board &b) {
  count = 0;
  b.gen_all_moves (*this);
}

std::ostream &
operator<< (std::ostream &os, const Move_Vector &moves)
{
  for (int i = 0; i < moves.count; i++)
    {
      os << moves.move[i] << endl;
    }
  return os;
}

/*************/
/* Constants */
/*************/

const string Board::INITIAL_POSITIONS =

//      FILE
// 0 1 2 3 4 5 6 7
  "r n b q k b n r"  // 7
  "p p p p p p p p"  // 6
  ". . . . . . . ."  // 5 R
  ". . . . . . . ."  // 4 A
  ". . . . . . . ."  // 3 N
  ". . . . . . . ."  // 2 K
  "P P P P P P P P"  // 1
  "R N B Q K B N R"; // 0

/*************************************/
/* Initialization of static members. */
/*************************************/

bool Board::have_precomputed_tables = false;

bitboard *Board::KNIGHT_ATTACKS_TBL = 0;
bitboard *Board::KING_ATTACKS_TBL = 0;
bitboard *Board::RANK_ATTACKS_TBL = 0;
bitboard *Board::FILE_ATTACKS_TBL = 0 ;
bitboard *Board::DIAG_45_ATTACKS_TBL = 0;
bitboard *Board::DIAG_135_ATTACKS_TBL = 0;

bitboard *Board::masks_0 = 0;
bitboard *Board::masks_45 = 0;
bitboard *Board::masks_90 = 0;
bitboard *Board::masks_135 = 0;

int *Board::rot_45 = 0;
int *Board::rot_90 = 0;
int *Board::rot_135 = 0;

byte *Board::diag_shifts_45 = 0;
byte *Board::diag_bitpos_45 = 0;
byte *Board::diag_widths_45 = 0;

byte *Board::diag_shifts_135 = 0;
byte *Board::diag_bitpos_135 = 0;
byte *Board::diag_widths_135 = 0;

// Initialize tables.
const void
Board::precompute_tables () {
  masks_0 = init_masks_0 ();
  masks_45 = init_masks_45 ();
  masks_90 = init_masks_90 ();
  masks_135 = init_masks_135 ();

  rot_45 = init_rot_45 ();
  rot_90 = init_rot_90 ();
  rot_135 = init_rot_135 ();

  diag_shifts_45 = init_diag_shifts_45 ();
  diag_bitpos_45 = init_diag_bitpos_45 ();
  diag_widths_45 = init_diag_widths_45 ();

  diag_shifts_135 = init_diag_shifts_135 ();
  diag_bitpos_135 = init_diag_bitpos_135 ();
  diag_widths_135 = init_diag_widths_135 ();

  KNIGHT_ATTACKS_TBL = init_knight_attacks_tbl ();
  KING_ATTACKS_TBL = init_king_attacks_tbl ();
  RANK_ATTACKS_TBL = init_rank_attacks_tbl ();
  FILE_ATTACKS_TBL = init_file_attacks_tbl ();
  DIAG_45_ATTACKS_TBL = init_45d_attacks_tbl ();
  DIAG_135_ATTACKS_TBL = init_135d_attacks_tbl ();

  have_precomputed_tables = true;
}

/*****************/
/* Constructors. */
/****************/

void
Board::common_init (Board &b) {

  // Initialize bitboards.
  b.white = b.black = 0x0;
  b.pawns = b.rooks = b.knights = b.bishops = b.queens = b.kings = 0x0;
  b.occupied = b.occupied_45 = b.occupied_90 = b.occupied_135 = 0x0;

  // Initialize flags.
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

  // Initialize game state.
  b.flags.status = GAME_IN_PROGRESS;

  // Initialize clocks.
  b.half_move_clock =
    b.full_move_clock = 0;
}

// Initialize a board from an ascii art representation of a board
// string.
Board
Board::from_ascii (const string &str) {

  Board b;

  // Do common initialization.
  Board::common_init (b);

  // The string we've been passed is in human readable format. To
  // convert from string offsets to bit offsets requires we need to
  // do the following transformation:
  //
  //    "x y z"
  //    ". . ."   = "x y z . . . X Y Z" => "X Y Z . . . x y z"
  //    "X Y Z"      0 1 2 3 4 5 6 7 8      0 1 2 3 4 5 6 7 8

  // Strip out padding characters.
  string piece_chars = "PRNBQKprnbqk.";
  string tmp = "";
  for (uint i = 0; i < str.length(); i++)
    if (piece_chars.find (str[i]) != string::npos)
      tmp += str[i];

  // Sanity check input.
  if (tmp.length () != 64)
    throw "Fatal Error.";

  // Reverse ranks.
  string bit_ordered = "";
  for (int y = 7; y >= 0; y--)
    for (int x = 0; x < 8; x++)
      bit_ordered += tmp[x + 8 * y];

  // Initialize bit boards.
  for (uint i = 0; i < bit_ordered.length (); i++)
    {
      switch (bit_ordered[i])
	{
	case 'P': b.set_piece (PAWN, WHITE, i); break;
	case 'p': b.set_piece (PAWN, BLACK, i); break;
	case 'N': b.set_piece (KNIGHT, WHITE, i); break;
	case 'n': b.set_piece (KNIGHT, BLACK, i); break;
	case 'B': b.set_piece (BISHOP, WHITE, i); break;
	case 'b': b.set_piece (BISHOP, BLACK, i); break;
	case 'Q': b.set_piece (QUEEN, WHITE, i); break;
	case 'q': b.set_piece (QUEEN, BLACK, i); break;
	case 'R': b.set_piece (ROOK, WHITE, i); break;
	case 'r': b.set_piece (ROOK, BLACK, i); break;

	case 'K':
	  b.set_piece (KING, WHITE, i);
	  if (i != 4)
	    b.flags.w_can_q_castle = b.flags.w_can_k_castle = 0;
	  break;

	case 'k':
	  b.set_piece (KING, BLACK, i);
	  if (i != 60)
	    b.flags.b_can_q_castle =
	      b.flags.b_can_k_castle = 0;
	  break;
	}
    }

  return b;
}

// Construct a board from the standard starting position.
Board
Board::startpos () {
  static const string position =

  //      FILE
  // 0 1 2 3 4 5 6 7
    "r n b q k b n r"  // 7
    "p p p p p p p p"  // 6
    ". . . . . . . ."  // 5 R
    ". . . . . . . ."  // 4 A
    ". . . . . . . ."  // 3 N
    ". . . . . . . ."  // 2 K
    "P P P P P P P P"  // 1
    "R N B Q K B N R"; // 0

  return from_ascii (position);
}

// Note that only the first six tokens in toks are examined, so we can
// reuse this code in the EPD parser.
Board
Board::from_fen (const string_vector &toks) {

  /*********************************************************************/
  /* Forsyth-Edwards Notation:                                         */
  /*                                                                   */
  /* Example: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 */
  /*********************************************************************/

  Board b;
  Board::common_init(b);

  // Specification from Wikipedia.
  // http://en.wikipedia.org/wiki/Forsyth-Edwards_Notation
  //
  // A FEN record contains six fields. The separator between fields is
  // a space. The fields are:

  assert (toks.size () >= 6);

  // 1. Piece placement (from white's perspective). Each rank is
  // described, starting with rank 8 and ending with rank 1; within
  // each rank, the contents of each square are described from file a
  // through file h. Following the Standard Algebraic Notation (SAN),
  // each piece is identified by a single letter taken from the
  // standard English names (pawn = "P", knight = "N", bishop = "B",
  // rook = "R", queen = "Q" and king = "K")[1]. White pieces are
  // designated using upper-case letters ("PNBRQK") while Black take
  // lowercase ("pnbrqk"). Blank squares are noted using digits 1
  // through 8 (the number of blank squares), and "/" separate ranks.

  int row = 7, file = 0;
  string::const_iterator i;
  
  for (i = toks[0].begin (); i < toks[0].end (); i++)
    {
      // Handle a piece code.
      if (isalpha (*i)) {
	b.set_piece 
	  (to_kind (*i), isupper (*i) ? WHITE : BLACK, row, file++);
      }

      // Handle count of empty squares.
      if (isdigit (*i)) { file += atoi (*i); }

      // Handle end of row.
      if (*i == '/') { row--; file = 0;}
    }

  // 2. Active color. "w" means white moves next, "b" means black.

  b.flags.to_move = tolower (toks[1][0]) == 'w' ? WHITE : BLACK;
   
  // 3. Castling availability. If neither side can castle, this is
  // "-". Otherwise, this has one or more letters: "K" (White can
  // castle kingside), "Q" (White can castle queenside), "k" (Black
  // can castle kingside), and/or "q" (Black can castle queenside).

  b.flags.w_can_q_castle = b.flags.w_can_k_castle = 
    b.flags.b_can_q_castle = b.flags.b_can_k_castle = 0;
   
  for (i = toks[2].begin (); i < toks[2].end (); i++)
    {
      switch (*i)
	{
	case 'K': b.flags.w_can_k_castle = 1; break;
	case 'Q': b.flags.w_can_q_castle = 1; break;
	case 'k': b.flags.b_can_k_castle = 1; break;
	case 'q': b.flags.b_can_q_castle = 1; break;
	}
    }

  // 4. En passant target square in algebraic notation. If there's no
  // en passant target square, this is "-". If a pawn has just made a
  // 2-square move, this is the position "behind" the pawn.

  if (is_number (toks[3])) b.flags.en_passant = to_int (toks[3]);
   
  // 5. Halfmove clock: This is the number of halfmoves since the last
  // pawn advance or capture. This is used to determine if a draw can
  // be claimed under the fifty-move rule.

  if (is_number (toks[4])) b.half_move_clock = to_int (toks[4]);

  // 6. Fullmove number: The number of the full move. It starts at 1,
  // and is incremented after Black's move.

  if (is_number (toks[5])) b.full_move_clock = to_int (toks[5]);

  return b;
}

// Construct from FEN string.
Board
Board::from_fen (const string &fen) {
  return from_fen (tokenize (fen));
}

/*********/
/* Tests */
/*********/

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
      attacks |= ((pieces & ~file(0)) << 7) & black;
      attacks |= ((pieces & ~file(7)) << 9) & black;
    }
  else
    {
      attacks |= ((pieces & ~file(0)) >> 9) & white;
      attacks |= ((pieces & ~file(7)) >> 7) & white;
    }

  // Rooks
  pieces = rooks & color;
  while (pieces)
    {
      from = bit_idx (pieces);
      attacks |=
	(RANK_ATTACKS_TBL[from * 256 + occ_0 (from)] |
	 FILE_ATTACKS_TBL[from * 256 + occ_90 (from)])
	& ~color;
      pieces = clear_lsb (pieces);
    }

  // Knights
  pieces = knights & color;
  while (pieces)
    {
      from = bit_idx (pieces);
      attacks |= KNIGHT_ATTACKS_TBL[from] & ~color;
      pieces = clear_lsb (pieces);
    }

  // Bishops
  pieces = bishops & color;
  while (pieces)
    {
      from = bit_idx (pieces);
      attacks |=
	(DIAG_45_ATTACKS_TBL[256 * from + occ_45 (from)] |
	 DIAG_135_ATTACKS_TBL[256 * from + occ_135 (from)])
	& ~color;
      pieces = clear_lsb (pieces);
    }

  // Queens
  pieces = queens & color;
  while (pieces)
    {
      from = bit_idx (pieces);
      attacks |=
	(RANK_ATTACKS_TBL[256 * from + occ_0 (from)]
	 | FILE_ATTACKS_TBL[256 * from + occ_90 (from)]
	 | DIAG_45_ATTACKS_TBL[256 * from + occ_45 (from)]
	 | DIAG_135_ATTACKS_TBL[256 * from + occ_135 (from)])
	& ~color;
      pieces = clear_lsb (pieces);
    }

  // Kings
  pieces = kings & color;
  while (pieces)
    {
      from = bit_idx (pieces);
      attacks |= KING_ATTACKS_TBL[from] & ~color;
      pieces = clear_lsb (pieces);
    }

  return attacks;
}

/********************/
/* Move application */
/********************/

// Apply a move to the board. Return false if this move is illegal
// because it places or leaves the color to move in check.
bool
Board::apply (const Move &m) {

  /******************/
  /* Update clocks. */
  /******************/

  // Reset the 50 move rule clock when a pawn is moved or a capture is
  // made.
  if (m.flags.capture != NULL_KIND || m.kind == PAWN)
    {
      half_move_clock = 0;
    }
  else
    {
      half_move_clock++;
    }

  // 50 move rule.
  if (half_move_clock > 50)
    {
      return false;
    }

  // Increment the whole move clock after each move by black.
  if (m.color == BLACK)
    {
      full_move_clock++;
    }

  /*************************/
  /* Update to_move state. */
  /*************************/

  flags.to_move = invert_color (flags.to_move);

  /****************************/
  /* Handling castling moves. */
  /****************************/

  if (m.flags.castle_qs || m.flags.castle_ks)
    {
      // Calculate attacked set for testing check.
      const bitboard attacked = attack_set (invert_color (m.color));

      // Test castle out of, across, or in to check.
      if (m.color == WHITE)
	{
	  if (m.flags.castle_qs && !(attacked & 0x1C))
	    {
	      clear_piece (4);
	      clear_piece (0);
	      set_piece (KING, WHITE, 2);
	      set_piece (ROOK, WHITE, 3);
	      flags.w_can_q_castle = 0;
	      flags.w_can_k_castle = 0;
	      flags.w_has_k_castled = 1;
	      return true;
	    }

	  if (m.flags.castle_ks && !(attacked & 0x70))
	    {
	      clear_piece (4);
	      clear_piece (7);
	      set_piece (KING, WHITE, 6);
	      set_piece (ROOK, WHITE, 5);
	      flags.w_can_q_castle = 0;
	      flags.w_can_k_castle = 0;
	      flags.w_has_k_castled = 1;
	      return true;
	    }
	}
      else
	{
	  byte attacks = get_byte (attacked, 7);

	  if (m.flags.castle_qs && !(attacks & 0x1C))
	    {
	      clear_piece (60);
	      clear_piece (56);
	      set_piece (KING, BLACK, 58);
	      set_piece (ROOK, BLACK, 59);
	      flags.b_can_q_castle = 0;
	      flags.b_can_k_castle = 0;
	      flags.b_has_q_castled = 1;
	      return true;
	    }

	  if (m.flags.castle_ks && !(attacks & 0x70))
	    {
	      clear_piece (60);
	      clear_piece (63);
	      set_piece (KING, BLACK, 62);
	      set_piece (ROOK, BLACK, 61);
	      flags.b_can_q_castle = 0;
	      flags.b_can_k_castle = 0;
	      flags.b_has_k_castled = 1;
	      return true;
	    }
	}

      return false;
    }
  else
    {
      /****************************/
      /* Handle taking En Passant */
      /****************************/

      if (m.kind == PAWN &&
	  flags.en_passant != 0 &&
	  m.to == flags.en_passant)
	{
	  // Clear the square behind the En Passant destination
	  // square.
	  if (m.color == WHITE)
	    {
	      clear_piece (flags.en_passant - 8);
	    }
	  else
	    {
	      clear_piece (flags.en_passant + 8);
	    }
	}

      /*********************************/
      /* Handle the non-castling case. */
      /*********************************/

      clear_piece (m.from);
      clear_piece (m.to);

      /**********************/
      /* Handle promotions. */
      /**********************/
      if (m.flags.promote == 0)
	{
	  set_piece (m.kind, m.color, m.to);
	}
      else
	{
	  set_piece (m.flags.promote, m.color, m.to);
	}

      /**************************/
      /* Update castling status */
      /**************************/

      // King moves.
      if (m.from == 4) {
	flags.w_can_q_castle = 0;
	flags.w_can_k_castle = 0;
      }
      else if (m.from == 60) {
	flags.b_can_q_castle = 0;
	flags.b_can_k_castle = 0;
      }

      // Rook moves and attacks.
      if (m.from ==  0 || m.to ==  0) { flags.w_can_q_castle = 0; }
      if (m.from ==  7 || m.to ==  7) { flags.w_can_k_castle = 0; }
      if (m.from == 56 || m.to == 56) { flags.b_can_q_castle = 0; }
      if (m.from == 63 || m.to == 63) { flags.b_can_k_castle = 0; }

      /************************************/
      /* Update En Passant target square. */
      /************************************/

      flags.en_passant = 0;
      if (m.kind == PAWN)
	{
	  if (m.color == WHITE)
	    {
	      if ((idx_to_rank (m.from) == 1) && (idx_to_rank (m.to) == 3))
		{
		  flags.en_passant = m.from + 8;
		}
	    }
	  else
	    {
	      if ((idx_to_rank (m.from) == 6) && (idx_to_rank (m.to) == 4))
		{
		  flags.en_passant = m.from - 8;
		}
	    }
	}

      /*****************/
      /* Test legality. */
      /*****************/

      // Return whether this position puts or leaves us in check.
      return !in_check (m.color);
    }
}

/******************/
/* Board vectors. */
/******************/

Board_Vector::Board_Vector (const Board &b)
{
  Move_Vector moves (b);

  count = 0;
  for (int i = 0; i < moves.count; i++)
    {
      board[count] = b;
      if (board[count].apply (moves[i]))
	{
	  count++;
	}
    }
}

std::ostream &
operator<< (std::ostream &os, Board_Vector bv) {
  for (int i = 0; i < bv.count; i++)
    {
      os << "Position #" << i << endl;
      os << bv[i] << endl;
    }
  return os;
}

/*************/
/* Printing. */
/*************/

std::ostream &
operator<< (std::ostream &os, const Board &b)
{
  if (b.flags.to_move == WHITE)
    {
      os << "White to move:" << endl;
    }
  else
    {
      os << "Black to move:" << endl;
    }
  
  os << "Castling: ";
  if (b.flags.w_can_k_castle) os << "K";
  if (b.flags.w_can_q_castle) os << "Q";
  if (b.flags.b_can_k_castle) os << "k";
  if (b.flags.b_can_q_castle) os << "q";
  os << endl;


  os << b.full_move_clock << " full moves, ";
  os << b.half_move_clock << " half moves." << endl;
  os << "En Passant square at " << b.flags.en_passant << endl;

  for (int y = 7; y >= 0; y--)
    {
      for (int x = 0; x < 8; x++)
	{
	  int offset = y * 8 + x;
	  char code = '.';
	  if (test_bit (b.pawns, offset)) code = 'P';
	  if (test_bit (b.rooks, offset)) code = 'R';
	  if (test_bit (b.knights, offset)) code = 'N';
	  if (test_bit (b.bishops, offset)) code = 'B';
	  if (test_bit (b.queens, offset)) code = 'Q';
	  if (test_bit (b.kings, offset)) code = 'K';
	  if (test_bit (b.black, offset)) code = tolower (code);
	  os << code << " ";
	}
      os << endl;
    }
  return os;
}
