/*
   Representation and operations on a board state in a game of chess.

   Matthew Gingell
   gingell@adacore.com
*/

#include <cassert>
#include <ctype>
#include "chesley.hpp"

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
      cerr  << *this << endl;
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

ostream & 
operator<< (ostream &os, Status s) {
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

ostream & 
operator<< (ostream &os, Kind k) {
  const char *strs[] =
    { "PAWN", "ROOK", "KNIGHT", "BISHOP", "KING", "QUEEN" };
  return os << strs [k];
}

/*****************/
/*  Color type.  */
/*****************/

ostream & operator<< (ostream &os, Color c) {
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

Move_Vector::Move_Vector (const Board &b) {
  count = 0;
  b.gen_all_moves (*this);
}

ostream &
operator<< (ostream &os, const Move &m)
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

ostream &
operator<< (ostream &os, const Move_Vector &moves)
{
  for (int i = 0; i < moves.count; i++)
    {
      os << value (moves.move[i]) << ":";
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

// Construct from FEN string.
Board 
Board::from_fen (const string &fen) {
  Board b;
  Board::common_init(b);
  const char *s = fen.c_str ();

  /*
    Forsyth-Edwards Notation:

    Example: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1

    Six fields:

    1: Piece placement, using digits to represent contiguous empty
       squares.
  */

  int file = 0;
  int row = 7;
  while (1)
    {
      // Do our best if the string ends unexpectantly.
      if (*s == 0) 
	{
	  return b;
	}

      // Skip space in the string.
      while (isspace (*s)) 
	{
	  s++;
	}

      // Process empty square counts.
      if (isnumber (*s))
	{
	  for (int j = 0; j < atoi (s); j++)
	    {
	      file++;
	    }
	}

      // Piece codes.
      switch (*s)
	{
	case 'P': b.set_piece (PAWN,   WHITE, row, file++); break;
	case 'p': b.set_piece (PAWN,   BLACK, row, file++); break;
	case 'R': b.set_piece (ROOK,   WHITE, row, file++); break;
	case 'r': b.set_piece (ROOK,   BLACK, row, file++); break;
	case 'N': b.set_piece (KNIGHT, WHITE, row, file++); break;
	case 'n': b.set_piece (KNIGHT, BLACK, row, file++); break;
	case 'B': b.set_piece (BISHOP, WHITE, row, file++); break;
	case 'b': b.set_piece (BISHOP, BLACK, row, file++); break;
	case 'K': b.set_piece (KING,   WHITE, row, file++); break;
	case 'k': b.set_piece (KING,   BLACK, row, file++); break;
	case 'Q': b.set_piece (QUEEN,  WHITE, row, file++); break;
	case 'q': b.set_piece (QUEEN,  BLACK, row, file++); break;
	}

      // End of row.
      if (*s == '/') 
	{
	  row--;
	  file = 0;
	}

      // Exit when we've filled the whole board.
      if (row == 0 && file > 7)
	break;
      
      s++;
    }

  /*
    2: Color to play. 'w' or 'b'.
  */

  // Scan to next field.
  while (!isspace (*s)) s++;
  while (isspace (*s)) s++;

  switch (*s)
    {
    case 'W':
    case 'w': b.flags.to_move = WHITE; break;
    case 'B':
    case 'b': b.flags.to_move = BLACK; break;
    }

  s++;

  /*
    3: Castling availability. "K" for kingside "Q" for queen size. "-"
       if neither side can castle.
  */

  // Scan to next field.
  while (!isspace (*s)) s++;
  while (isspace (*s)) s++;;

  b.flags.w_can_k_castle = 
    b.flags.w_can_q_castle = 
    b.flags.w_can_k_castle = 
    b.flags.w_can_q_castle = 0;
    
  int done = false;
  while (!done) 
    {
      switch (*s) 
	{
	case 'K': b.flags.w_can_k_castle = 1; break;
	case 'k': b.flags.b_can_k_castle = 1; break;
	case 'Q': b.flags.w_can_q_castle = 1; break;
	case 'q': b.flags.b_can_q_castle = 1; break;
	case '-': break;
	default: done = true; break;
	}
      s++;
    }
  
  /*
      4: En passant target square. If a pawn has just made a 2-square
         move this is the postion behind the pawn, otherwise "-".
  */

  // Scan to next field.
  while (isspace (*s)) s++;

  if (*s == '-') 
    {
      b.flags.en_passant = 0;
    }
  else   
    {
      int tmp;
      sscanf (s, "%i", &tmp);
      b.flags.en_passant = tmp;
    }
  
  // Scan to next field.
  while (!isspace (*s)) s++;
  while (isspace (*s)) s++;

  /*
    5: Halfmove clock. Number of half moves since the last pawn
    advance or capture. Used to implemenent 50-move rule.

    6: Fullmove number: Starts at one an is incremented after black
    moves.
  */

  sscanf (s, "%i %i", &b.half_move_clock, &b.full_move_clock);
  
  return b;
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
	  if (m.flags.castle_qs && !(attacked & 0xE))
	    {
	      clear_piece (0);
	      clear_piece (4);
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
	   
	  if (m.flags.castle_qs && !(attacks & 0xE))
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

ostream & 
operator<< (ostream &os, Board_Vector bv) {
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

ostream &
operator<< (ostream &os, const Board &b)
{
  if (b.flags.to_move == WHITE)
    {
      os << "White to move:" << endl;
	
    }
  else
    {
      os << "Black to move:" << endl;
    }

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
