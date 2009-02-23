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

  init_zobrist_keys ();

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

  // Initialize hash value;
  b.hash = 0x0;

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
Board::child_count () {
  return (Board_Vector (*this)).count;
}

// Get the status of the game. {
Status 
Board::get_status () {
  if (child_count () > 0) return GAME_IN_PROGRESS;
  if (in_check (WHITE)) return GAME_WIN_BLACK;
  if (in_check (BLACK)) return GAME_WIN_WHITE;
  return GAME_DRAW;
}

// Return whether color c is in check. 
bool 
Board::in_check (Color c) const
{
  // Take advantage of the symmetry that if a king could move like an
  // X and capture an X, then that X is able to attack us and we are
  // in check.

  bitboard king = kings & color_to_board (c);
  bitboard them = color_to_board (invert_color (c));
  bitboard attacks;
  int from = bit_idx (king);

  // Are we attacked along a rank?
  attacks = RANK_ATTACKS_TBL[from * 256 + occ_0 (from)];
  if (attacks & (them & (queens | rooks))) return true;  

  // Are we attack along a file?
  attacks = FILE_ATTACKS_TBL[from * 256 + occ_90 (from)];
  if (attacks & (them & (queens | rooks))) return true;  

  // Are we attacked along a 45 degree diagonal.
  attacks = DIAG_45_ATTACKS_TBL[256 * from + occ_45 (from)];
  if (attacks & (them & (queens | bishops))) return true;  

  // Are we attacked along a 135 degree diagonal.
  attacks = DIAG_135_ATTACKS_TBL[256 * from + occ_135 (from)];
    if (attacks & (them & (queens | bishops))) return true;  

  // Are we attacked by a knight?
  attacks = KNIGHT_ATTACKS_TBL[from];
  if (attacks & (them & knights)) return true;

  // Are we attacked by a king?
  attacks = KING_ATTACKS_TBL[from];
  if (attacks & (them & kings)) return true;

  // Are we attacked by a pawn?
  bitboard their_pawns = pawns & them;
  if (c == WHITE) 
    {
      attacks = (((their_pawns & ~file (7)) >> 7)
                 | ((their_pawns & ~file (0)) >> 9));
    }
  else
    {
      attacks = (((their_pawns & ~file (0)) << 7) 
                 | ((their_pawns & ~file (7)) << 9));
    }

  if (attacks & king) return true;

  return false;
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
  if (half_move_clock > 50) return false;

  // Increment the whole move clock after each move by black.
  if (m.color == BLACK) full_move_clock++;

  /*************************/
  /* Update to_move state. */
  /*************************/

  if (flags.to_move == WHITE)
    {
      flags.to_move = BLACK;
      hash ^= zobrist_key_white;
      hash ^= zobrist_key_black;
    }
  else
    {
      flags.to_move = WHITE;
      hash ^= zobrist_key_black;
      hash ^= zobrist_key_white;
    }

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

  /*****************************************************************/
  /* Update En Passant target square. This needs to be done before */
  /* checking castling, since otherwise we may return without      */
  /* clearing the En Passant target square.                        */
  /*****************************************************************/

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
      if (m.from == 4) 
        {
          flags.w_can_q_castle = 0;
          flags.w_can_k_castle = 0;
        }
      else if (m.from == 60) 
        {
          flags.b_can_q_castle = 0;
          flags.b_can_k_castle = 0;
        }

      // Rook moves and attacks.
      if (m.from ==  0 || m.to ==  0) { flags.w_can_q_castle = 0; }
      if (m.from ==  7 || m.to ==  7) { flags.w_can_k_castle = 0; }
      if (m.from == 56 || m.to == 56) { flags.b_can_q_castle = 0; }
      if (m.from == 63 || m.to == 63) { flags.b_can_k_castle = 0; }

      /*****************/
      /* Test legality. */
      /*****************/

      // Return whether this position puts or leaves us in check.
      return !in_check (m.color);
    }
}

/***********************************/
/* Move vectors and board vectors. */
/***********************************/

Move_Vector::Move_Vector (const Board &b) {
  count = 0;
  b.gen_all_moves (*this);
}

Board_Vector::Board_Vector (const Board &b)
{
  Move_Vector moves (b);

  count = 0;
  for (int i = 0; i < moves.count; i++)
    {
      board[count] = b;
      if (board[count].apply (moves[i])) count++;
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

// Print a board vector.
ostream &
operator<< (ostream &os, Board_Vector bv) {
  for (int i = 0; i < bv.count; i++)
    os << "Position #" << i << bv[i] << endl;
  return os;
}

// Print a move vector
ostream &
operator<< (ostream &os, const Move_Vector &moves)
{
  for (int i = 0; i < moves.count; i++)
    {
      os << moves.move[i] << endl;
    }
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
  Kind k = get_kind (from);
  Color c = flags.to_move;
  Kind capture = get_kind (to);
  Move m = Move (k, from, to, c, capture);

  // Check for promotion.
  if (s.length () >= 5)
    {
      assert (k == PAWN);
      m.flags.promote = to_kind (s[4]);
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

  if (toks[3][0] != '-')
    {
      b.flags.en_passant = (toks[3][0] - 'a') + 8 * ((toks[3][1] - '0') - 1);
    }

  if (!EPD) 
    {
   
      // 5. Halfmove clock: This is the number of halfmoves since the last
      // pawn advance or capture. This is used to determine if a draw can
      // be claimed under the fifty-move rule.
      
      if (is_number (toks[4])) b.half_move_clock = to_int (toks[4]);
      
      // 6. Fullmove number: The number of the full move. It starts at 1,
      // and is incremented after Black's move.
      
      if (is_number (toks[5])) b.full_move_clock = to_int (toks[5]);
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

/**********/
/* Debug. */
/**********/

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
