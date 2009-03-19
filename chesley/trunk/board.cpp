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

/*****************/
/*  Color type.  */
/*****************/

std::ostream &
operator<< (std::ostream &os, Color c) {
  switch (c)
    {
    case WHITE: return os << "WHITE";
    case BLACK: return os << "BLACK";
    default: return os;
    }
}

/******************/
/* Board::History */
/******************/

// Manage a history associated with a state of place and location in
// a search tree. 
// 
// We manage bookkeeping for two types of information: The log of
// all the positions and moves which have actually been played, and
// a temporary set of board hash signature counts which are
// incremented and decremented during as a particular traversal of a
// search tree. 
//
// This distinction is only made for efficiency purposes and to
// minimize the amount of work done in the actual tree search
// itself.

// Increment the number of times 'b' occurs in a search path.
void 
Board::History::push (const Board &b) {
  boost::unordered_map <hash_t, uint32> :: iterator i;
  i = counts.find (b.hash);
  if (i == counts.end()) {
    counts[b.hash] = 1;
  } else {
    i -> second++;
  }
}

// Decrement the number of times 'b' occurs in a search path.
void 
Board::History::pop (const Board &b) {
  boost::unordered_map <hash_t, uint32> :: iterator i;
  i = counts.find (b.hash);
  if (i == counts.end()) {
    assert (false);
  } else {
    i -> second--;
    if (i -> second == 0)
      {
	counts.erase (i);
      }
  }   
}

// Determine whether 'b' is the third repitition of a position in this
// history.
bool 
Board::History::is_triple_repetition (const Board &b) const {
  boost::unordered_map <hash_t, uint32> :: iterator i;
 
#if 0
  cerr << "checking whether " << b.hash << " is a tripple rep:" << endl;
  for (i = counts.begin (); i != counts.end (); i++)
    {
      cerr << i -> first << ":" << i -> second << endl;
    }
  cerr << endl;
#endif

  //  i = counts.find (b.hash);
  if (i == counts.end()) {
    return false;
  } else {

    if (i -> second == 2) 
      {
	cerr << "Found triple repetition." << endl;
      }

    return (i -> second == 2);
  }   
}

// Commit a move to the history, storing all the information
// associated with it to the permanent log for this state of play.
void 
Board::History::commit (const Board &b, const Move &m) {
  // Append this board to the positions list.
  positions.push_back (b);

  // Append this move to the moves list.  
  moves.push_back (m);

  // Add this board to the repetitions table.
  push (b);
}

/*************/
/* Constants */
/*************/

const string Board::
INITIAL_POSITIONS = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

#if 0
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
#endif

/*****************/
/* Constructors. */
/****************/

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
      attacks |= ((pieces & ~file(0)) << 7);
      attacks |= ((pieces & ~file(7)) << 9);
    }
  else
    {
      attacks |= ((pieces & ~file(0)) >> 9);
      attacks |= ((pieces & ~file(7)) >> 7);
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

// Get the number of legal moves available from this position.
int
Board::child_count () const {
  return (Board_Vector (*this)).count;
}

// Get the status of the game. {
Status
Board::get_status () const {
  if (half_move_clock >= 50) return GAME_DRAW;
  if (child_count () > 0) return GAME_IN_PROGRESS;
  if (in_check (WHITE)) return GAME_WIN_BLACK;
  if (in_check (BLACK)) return GAME_WIN_WHITE;
  return GAME_DRAW;
}

// Return whether the square at idx is attacked by a piece of color c.
bool
Board::is_attacked (int idx, Color c) const
{
  // Take advantage of the symmetry that if the piece at index could
  // move like an X and capture an X, then that X is able to attack it

  bitboard them = color_to_board (c);
  bitboard attacks;
  
  // Are we attacked along a rank?
  attacks = RANK_ATTACKS_TBL[idx * 256 + occ_0 (idx)];
  if (attacks & (them & (queens | rooks))) return true;

  // Are we attack along a file?
  attacks = FILE_ATTACKS_TBL[idx * 256 + occ_90 (idx)];
  if (attacks & (them & (queens | rooks))) return true;

  // Are we attacked along a 45 degree diagonal.
  attacks = DIAG_45_ATTACKS_TBL[256 * idx + occ_45 (idx)];
  if (attacks & (them & (queens | bishops))) return true;

  // Are we attacked along a 135 degree diagonal.
  attacks = DIAG_135_ATTACKS_TBL[256 * idx + occ_135 (idx)];
    if (attacks & (them & (queens | bishops))) return true;

  // Are we attacked by a knight?
  attacks = KNIGHT_ATTACKS_TBL[idx];
  if (attacks & (them & knights)) return true;

  // Are we attacked by a king?
  attacks = KING_ATTACKS_TBL[idx];
  if (attacks & (them & kings)) return true;

  // Are we attacked by a pawn?
  bitboard their_pawns = pawns & them;
  if (c != WHITE)
    {
      attacks = (((their_pawns & ~file (7)) >> 7)
                 | ((their_pawns & ~file (0)) >> 9));
    }
  else
    {
      attacks = (((their_pawns & ~file (0)) << 7)
                 | ((their_pawns & ~file (7)) << 9));
    }
  if (attacks & masks_0[idx]) return true;

  return false;
}

// Return whether color c is in check.
bool
Board::in_check (Color c) const
{
  int idx = bit_idx (kings & color_to_board (c));
  assert (idx >= 0 && idx < 64);
  return is_attacked (idx, invert_color (c));
}

/***************************/
/* Seting castling rights  */
/***************************/

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

/********************/
/* Move application */
/********************/

// Apply a move to the board. Return false if this move is illegal
// because it places or leaves the color to move in check.
bool
Board::apply (const Move &m) {

  Kind kind = m.get_kind (*this);
  Kind capture = m.capture (*this);
  Color color = m.get_color (*this);
  bool is_castle_qs = m.is_castle_qs (*this);
  bool is_castle_ks = m.is_castle_ks (*this);
  bool is_en_passant = m.is_en_passant (*this);

  /******************/
  /* Update clocks. */
  /******************/

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
  if (half_move_clock > 50)
    {
      return false;
    }

  /*************************/
  /* Update to_move state. */
  /*************************/

  set_color (invert_color (flags.to_move));

  /****************************/
  /* Handle taking En Passant */
  /****************************/

  if (is_en_passant)
    {
      // Clear the square behind destination square.
      if (color == WHITE)
	clear_piece (flags.en_passant - 8);
      else
	clear_piece (flags.en_passant + 8);
    }

  /*****************************************************************/
  /* Update En Passant target square. This needs to be done before */
  /* checking castling, since otherwise we may return without      */
  /* clearing the En Passant target square.                        */
  /*****************************************************************/

  set_en_passant (0);
  if (kind == PAWN)
    {
      if (color == WHITE) {
	if ((idx_to_rank (m.from) == 1) && (idx_to_rank (m.to) == 3))
	  set_en_passant (m.from + 8);
      } else {
	if ((idx_to_rank (m.from) == 6) && (idx_to_rank (m.to) == 4))
	  set_en_passant (m.from - 8);
      }
    }
    
  /****************************/
  /* Handling castling moves. */
  /****************************/

  if (is_castle_qs || is_castle_ks)
    {
      // Calculate attacked set for testing check.
      const bitboard attacked = attack_set (invert_color (color));

      // Test castle out of, across, or in to check.
      if (color == WHITE)
	{
	  if (is_castle_qs && !(attacked & 0x1C))
	    {
	      clear_piece (4);
	      clear_piece (0);
	      set_piece (KING, WHITE, 2);
	      set_piece (ROOK, WHITE, 3);
	      set_castling_right (W_QUEEN_SIDE, false);
	      set_castling_right (W_KING_SIDE, false);
	      flags.w_has_k_castled = 1;
	      return true;
	    }
	  if (is_castle_ks && !(attacked & 0x70))
	    {
	      clear_piece (4);
	      clear_piece (7);
	      set_piece (KING, WHITE, 6);
	      set_piece (ROOK, WHITE, 5);
	      set_castling_right (W_QUEEN_SIDE, false);
	      set_castling_right (W_KING_SIDE, false);
	      flags.w_has_k_castled = 1;
	      return true;
	    }
	}
      else
	{
	  byte attacks = get_byte (attacked, 7);
	  if (is_castle_qs && !(attacks & 0x1C))
	    {
	      clear_piece (60);
	      clear_piece (56);
	      set_piece (KING, BLACK, 58);
	      set_piece (ROOK, BLACK, 59);
	      set_castling_right (B_QUEEN_SIDE, false);
	      set_castling_right (B_KING_SIDE, false);
	      flags.b_has_q_castled = 1;
	      return true;
	    }
	  if (is_castle_ks && !(attacks & 0x70))
	    {
	      clear_piece (60);
	      clear_piece (63);
	      set_piece (KING, BLACK, 62);
	      set_piece (ROOK, BLACK, 61);
	      set_castling_right (B_QUEEN_SIDE, false);
	      set_castling_right (B_KING_SIDE, false);
	      flags.b_has_k_castled = 1;
	      return true;
	    }
	}

      return false;
    }
  else
    {
      /*********************************/
      /* Handle the non-castling case. */
      /*********************************/

      clear_piece (m.from);
      clear_piece (m.to);

      /**********************/
      /* Handle promotions. */
      /**********************/

      if (m.promote != NULL_KIND)
	set_piece (m.promote, color, m.to);
      else
	set_piece (kind, color, m.to);

      /**************************/
      /* Update castling status */
      /**************************/

      // King moves.
      if (m.from == 4)
	{
	  set_castling_right (W_QUEEN_SIDE, false);
	  set_castling_right (W_KING_SIDE, false);
	}
      else if (m.from == 60)
	{
	  set_castling_right (B_QUEEN_SIDE, false);
	  set_castling_right (B_KING_SIDE, false);
	}

      // Rook moves and attacks.

      if (m.from ==  0 || m.to ==  0) { set_castling_right (W_QUEEN_SIDE, false); }
      if (m.from ==  7 || m.to ==  7) { set_castling_right (W_KING_SIDE, false);  }
      if (m.from == 56 || m.to == 56) { set_castling_right (B_QUEEN_SIDE, false); }
      if (m.from == 63 || m.to == 63) { set_castling_right (B_KING_SIDE, false);  }

      /*****************/
      /* Test legality. */
      /*****************/
      
      // Return whether this position puts or leaves us in check.
      return !in_check (color);
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
      Board c = b;
      if (c.apply (moves [i])) push (c);
    }
}

/***********/
/*   I/O   */
/***********/

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
	    << (int) (m.from) << " => " 
	    << (int) (m.to) 
	    << " " << m.score << "]";
}

// Print a board vector.
ostream &
operator<< (ostream &os, Board_Vector bv) {
  for (int i = 0; i < bv.count; i++)
    os << "Position #" << i << bv[i] << endl;
  return os;
}

// Construct a Move from coordinate algebraic notation.
Move
Board::from_calg (const string &s) const {
  assert (s.length () >= 4);

  // Decode string
  uint32 from = (s[0] - 'a') + 8 * (s[1] - '1');
  uint32 to   = (s[2] - 'a') + 8 * (s[3] - '1');

  // Build move.
  Move m = Move (from, to, (s.length () >= 5) ? to_kind (s[4]) : NULL_KIND);

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
  if (m.promote != NULL_KIND)
    {
      s << to_char (m.promote);
    }

  return s.str();
}

// Note that only the first six tokens in toks are examined. If EPD is
// true, then we are parsing an EPD command and the last two fields,
// clock and half clock, are not provided.
Board
Board::from_fen (const string_vector &toks, bool EPD) {

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

  if (!EPD) assert (toks.size () >= 6);

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

  b.set_color (tolower (toks[1][0]) == 'w' ? WHITE : BLACK);

  // 3. Castling availability. If neither side can castle, this is
  // "-". Otherwise, this has one or more letters: "K" (White can
  // castle kingside), "Q" (White can castle queenside), "k" (Black
  // can castle kingside), and/or "q" (Black can castle queenside).

  b.set_castling_right (W_QUEEN_SIDE, false);
  b.set_castling_right (W_KING_SIDE, false);
  b.set_castling_right (B_QUEEN_SIDE, false);
  b.set_castling_right (B_KING_SIDE, false);

  for (i = toks[2].begin (); i < toks[2].end (); i++)
    {
      switch (*i)
        {
        case 'Q': b.set_castling_right (W_QUEEN_SIDE, true); break;
        case 'K': b.set_castling_right (W_KING_SIDE, true); break;
        case 'q': b.set_castling_right (B_QUEEN_SIDE, true); break;
        case 'k': b.set_castling_right (B_KING_SIDE, true); break;
        }
    }

  // 4. En passant target square in algebraic notation. If there's no
  // en passant target square, this is "-". If a pawn has just made a
  // 2-square move, this is the position "behind" the pawn.

  if (toks.size () >= 3) {
    if (toks[3][0] != '-') 
      b.set_en_passant 
	((toks[3][0] - 'a') + 8 * ((toks[3][1] - '0') - 1));
  } else {
    b.set_en_passant (0);
  }
  
  // These fields are not expected when we are parsing an EPD command.
  if (!EPD)
    {
      
      // 5. Halfmove clock: This is the number of halfmoves since the last
      // pawn advance or capture. This is used to determine if a draw can
      // be claimed under the fifty-move rule.
      
      if (toks.size () >= 4) {
	if (is_number (toks[4])) b.half_move_clock = to_int (toks[4]);
      } else {
	b.half_move_clock = 0;
      }

      // 6. Fullmove number: The number of the full move. It starts at 1,
      // and is incremented after Black's move.

      if (toks.size () >= 5) {
	if (is_number (toks[5])) b.full_move_clock = to_int (toks[5]);
      } else {
	b.full_move_clock = 0;
      }

    }

  return b;
}

// Construct from FEN string.
Board
Board::from_fen (const string &fen, bool EPD) {
  return from_fen (tokenize (fen), EPD);
}

// Return an ASCII representation of this position.
string
Board::to_ascii () const {
  ostringstream s;

  // Precede board diagram with status, formated as in FEN strings.
  s << (flags.to_move == WHITE ? 'w' : 'b') << " ";

  if (flags.w_can_k_castle | flags.w_can_q_castle |
      flags.b_can_k_castle | flags.b_can_q_castle)
    {
      if (flags.w_can_k_castle) s << 'K';
      if (flags.w_can_q_castle) s << 'Q';
      if (flags.b_can_k_castle) s << 'k';
      if (flags.b_can_q_castle) s << 'q';
    }
  else
    {
      s << '-';
    }

  s << ' ';

  if (flags.en_passant)
    {
      s << (char) ('a' + idx_to_file (flags.en_passant));
      s << (int)  (      idx_to_rank (flags.en_passant));
    }
  else
    {
      s << '-';
    }

  s << ' ' << half_move_clock << ' ' << full_move_clock << endl;

  // Dump a human readable ASCII art diagram of the board.
  for (int row = 7; row >= 0; row--)
    {
      for (int file = 0; file < 8; file++)
        {
          Kind k = get_kind (row, file);

          if (k == NULL_KIND)
            {
              s << ".";
            }
          else
            {
              if (get_color (row, file) == WHITE)
                {
                  s << (char) toupper (to_char (k));
                }
              else
                {
                  s << (char) tolower (to_char (k));
                }
            }

          if (file != 7) s << ' ';
        }
      if (row != 0) s << endl;
    }

    return s.str ();
}


// Return a FEN string for this position.
string
Board ::to_fen () const {
  ostringstream s;

  // A FEN record contains six fields. The separator between fields is
  // a space. The fields are:

  // 1. Piece placement (from white's perspective).
  for (int row = 7; row >= 0; row--)
    {
      int counter = 0;

      for (int file = 0; file < 8; file++)
        {
          Kind k = get_kind (row, file);

          if (k == NULL_KIND)
            {
              counter++;
            }
          else
            {
              char code = to_char (k);

              if (counter > 0)
                {
                  s << counter;
                  counter = 0;
                }

              if (get_color (row, file) == WHITE)
                {
                  s << (char) toupper (code);
                }
              else
                {
                  s << (char) tolower (code);
                }
            }
        }

      if (counter > 0) s << counter;
      if (row > 0) s << "/";
    }

  // 2. Active color. "w" means white moves next, "b" means black.

  s << " " << (flags.to_move == WHITE ? 'w' : 'b');

  // 3. Castling availability.

  s << ' ';

  if (flags.w_can_k_castle | flags.w_can_q_castle |
      flags.b_can_k_castle | flags.b_can_q_castle)
    {
      if (flags.w_can_k_castle) s << 'K';
      if (flags.w_can_q_castle) s << 'Q';
      if (flags.b_can_k_castle) s << 'k';
      if (flags.b_can_q_castle) s << 'q';
    }
  else
    {
      s << '-';
    }

  // 4. En passant target square in algebraic notation.

  s << ' ';

  if (flags.en_passant)
    {
      s << (char) ('a' + idx_to_file (flags.en_passant));
      s << (int)  (      idx_to_rank (flags.en_passant));
    }
  else
    {
      s << '-';
    }

  // 5. Halfmove clock.

  s << ' ' << half_move_clock;

  // 6. Fullmove clock.

  s << ' ' << full_move_clock;

  return s.str();
}

// Initialize a board from an ascii art representation of a board
// string.
Board
Board::from_ascii (const string &str) {
  Board b;

  Board::common_init (b);
  assert (0);
  return b;
}

/************/
/* Testing. */
/************/

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
      Board_Vector children (*this);
      for (int i = 0; i < children.count; i++)
	children[i].print_tree (depth - 1);
    }
}

// Generate a hash key from scratch. This is used to test the
// correctness of our incremental hash update code.
uint64
Board::gen_hash () const {
  uint64 h = 0x0;
  Color to_move = flags.to_move;

  // Set color to move.
  if (to_move == WHITE) h ^= zobrist_key_white_to_move;

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
