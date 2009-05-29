////////////////////////////////////////////////////////////////////////////////
// 								     	      //
// notation.cpp                                                               //
//                                                                            //
// This file implements operations on different kinds of string               //
// representation for Chess moves and positions. Included here are            //
// routines for parsing and generating Standard Algebraic Notation            //
// moves, Coordinate Algebraic Notation moves, and Forsyth-Edwards            //
// position notation.                                                         //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <cassert>
#include <sstream>
#include <string>

#include "board.hpp"

using namespace std;

//////////////////////////////////////////////////////////
// Construct a Move from Coordinate Algebraic Notation. //
//////////////////////////////////////////////////////////

Move
Board::from_calg (const string &s) const {
  assert (s.length () >= 4);

  // Decode string
  uint32 from = (s[0] - 'a') + 8 * (s[1] - '1');
  uint32 to   = (s[2] - 'a') + 8 * (s[3] - '1');

  // Build move.
  return Move (from, to, (s.length () >= 5) ? to_kind (s[4]) : NULL_KIND);
}

//////////////////////////////////////////////////////////////////////
// Return a description of a Move in Coordinate Algebraic Notation. //
//////////////////////////////////////////////////////////////////////

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
    s << to_char (m.promote);

  return s.str();
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// The description of SAN moves below is excerpted from:                //
//                                                                      //
//   _Standard: Portable Game Notation Specification and Implementation //
//   Guide_ Revised: 1994.03.12 Authors: Interested readers of the      //
//   Internet newsgroup rec.games.chess Coordinator: Steven J. Edwards  //
//   (send comments to sje@world.std.com)                               //
//                                                                      //
// see http:www.chessclub.com/help/PGN-spec                             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

/*
  8.2.3.3: Basic SAN move construction

  A basic SAN move is given by listing the moving piece letter
  (omitted for pawns) followed by the destination square.  Capture
  moves are denoted by the lower case letter "x" immediately prior to
  the destination square; pawn captures include the file letter of the
  originating square of the capturing pawn immediately prior to the
  "x" character.

  SAN kingside castling is indicated by the sequence "O-O"; queenside
  castling is indicated by the sequence "O-O-O".

  En passant captures do not have any special notation; they are
  formed as if the captured pawn were on the capturing pawn's
  destination square.

  Pawn promotions are denoted by the equal sign "=" immediately
  following the destination square with a promoted piece letter
  (indicating one of knight, bishop, rook, or queen) immediately
  following the equal sign.  As above, the piece letter is in upper
  case.

  8.2.3.4: Disambiguation

  In the case of ambiguities (multiple pieces of the same type moving
  to the same square), the first appropriate disambiguating step of
  the three following steps is taken:

  First, if the moving pieces can be distinguished by their
  originating files, the originating file letter of the moving piece
  is inserted immediately after the moving piece letter.

  Second (when the first step fails), if the moving pieces can be
  distinguished by their originating ranks, the originating rank
  digit of the moving piece is inserted immediately after the moving
  piece letter.

  Third (when both the first and the second steps fail), the two
  character square coordinate of the originating square of the moving
  piece is inserted immediately after the moving piece letter.

  8.2.3.5: Check and checkmate indication characters

  If the move is a checking move, the plus sign "+" is appended as a
  suffix to the basic SAN move notation; if the move is a checkmating
  move, the octothorpe sign "#" is appended instead.

  Neither the appearance nor the absence of either a check or
  checkmating indicator is used for disambiguation purposes.

  There are no special markings used for drawing moves.

  8.2.3.6: SAN move length

  SAN moves can be as short as two characters (e.g., "d4"), or as
  long as seven characters (e.g., "Qa6xb7#", "fxg1=Q+").

  8.2.3.8: SAN move suffix annotations

  Import format PGN allows for the use of traditional suffix
  annotations for moves.  There are exactly six such annotations
  available: "!", "?", "!!", "!?", "?!", and "??".  At most one such
  suffix annotation may appear per move, and if present, it is always
  the last part of the move symbol.
*/

// Produce an algebraic letter-number pair for an integer square
// number.
string
Board::to_alg_coord (int idx) const {
  ostringstream s;

  s << (char) ('a' + idx_to_file (idx));
  s << (int)  (      idx_to_rank (idx) + 1);
  return s.str ();
}

// Produce an an integer square number from an algebraic letter-number
// pair.
int
Board::from_alg_coord (const string &s) const {
  return (s[0] - 'a') + 8 * (s[1] - '1');
}

// Produce a SAN string from a move.
string
Board::to_san (const Move &m) const {
  ostringstream s;
  Kind k = m.get_kind (*this);
  Kind cap = m.capture (*this);
  Kind promote = m.promote;

  if (m.is_null ())
    {
      return "<null>";
    }


  if (k == NULL_KIND)
    {
      return "<san?>";
    }

  //////////////////////////////////////////
  // 8.2.3.3: Basic SAN move construction //
  //////////////////////////////////////////

  if (!m.is_castle (*this))
    {
      // Write letter for moving piece.
      if (k != PAWN) s << to_char (k);

      /////////////////////////////
      // 8.2.3.4: Disambiguation //
      /////////////////////////////

      // Determine whether this move is ambiguous.
      Move_Vector moves (*this);
      int from_file = idx_to_file (m.from);
      int from_rank = idx_to_rank (m.from);
      bool need_file = false, need_rank = false;
      for (int i = 0; i < moves.count; i++)
        {
          if (m == moves[i]) continue;

          Kind ki = moves[i].get_kind (*this);

          // Is another piece of the same kind is attacking the same
          // destination?
          if (k == ki && m.to == moves[i].to)
            {
              // If these two pieces are the same rank, we will
              // disambiguate by writing the file.
              if (from_rank == idx_to_rank (moves[i].from)) need_file = true;

              // If these two pieces are the same file, we will
              // disambiguate by writing the rank.
              if (from_file == idx_to_file (moves[i].from)) need_rank = true;

              // If these pieces are neither on the same rank nor the
              // same file, disambiguate using the file.
              if (!need_file && !need_rank) need_file = true;
            }
        }

      // Write out rank and file as required for disambiguation. Note
      // that we need to write the origination file for pawn captures
      // in any case.
      if (need_file || (k == PAWN && cap != NULL_KIND))
        s << (char) ('a' + from_file);
      if (need_rank)
        s << 1 + from_rank;

      // Denote captures.
      if (cap != NULL_KIND)
        s << "x";

      // Write destination.
      s << to_alg_coord (m.to);
    }
  else
    {
      // Write a castling move.
      if (m.is_castle_ks (*this))
        {
          return "O-O";
        }
      else
        {
          return "O-O-O";
        }
    }

  // Write pawn promotion.
  if (promote != NULL_KIND)
    s << "=" << to_char (promote);

  // Determine whether this is a check or checkmate.
  Board c = *this;
  bool applied = c.apply (m);
  assert (applied);
  if (c.in_check (c.to_move ()))
    {
      if (c.child_count () == 0)
        s << "#";
      else
        s << "+";
    }

  return s.str ();
}

// Produce a move from a SAN string.
Move
Board::from_san (const string &s) const {
  string::const_iterator i = s.begin ();
  Kind k = NULL_KIND;
  Kind promote = NULL_KIND;
  bool is_capture = false;
  int file = -1, rank = -1;
  int to = -1;
  int dis_rank = -1, dis_file = -1;
  
  // Handle the case of castling kingside.
  if (s == "O-O") 
    {
      return (to_move () == WHITE) ? Move (4, 6) : Move (60, 62);
    }

  // Handle the case of castling queenside.
  if (s == "O-O-O") 
    {
      return (to_move () == WHITE) ? Move (4, 2) : Move (60, 58);
    }

  // Otherwise, we expect an upper case piece code or a lower case
  // coordinate if the piece being moved is a pawn.
  if (i == s.end ()) 
    throw string ("from_san: failed parsing ") + s;

  if (isupper (*i))
    k = to_kind (*i++);
  else
    k = PAWN;

  if (i == s.end () || k == NULL_KIND)
    throw string ("from_san: failed parsing ") + s;

  // We now expect to read a coordinate which may be the origin or the
  // destination of this move, or a letter or number indicating the
  // file or rank of the origin.
  if (*i == 'x')  { is_capture = true; i++; }
  if (*i >= 'a' && *i <= 'h') file = *i++ - 'a';
  if (*i >= '1' && *i <= '8') rank = *i++ - '1';
  if (*i == 'x') { is_capture = true; i++; }

  // If we're at the end of the string or the next character doesn't
  // look like the start of a coordinate, we've just read the
  // destination square.
  if (i == s.end () || !(*i >= 'a' && *i <= 'h'))
    {
      to = file + 8 * rank;
    }

  // Otherwise, what we've just read is for disambiguating the origin
  // and we now expect to read the destiniation.
  else
    {
      dis_file = file;
      dis_rank = rank;
      if (*i == 'x')  { is_capture = true; i++; }
      if (*i >= 'a' && *i <= 'h') file = *i++ - 'a';
      if (*i >= '1' && *i <= '8') rank = *i++ - '1';   
      to = file + 8 * rank;
    }

  // Parse promotion information.
  if (i != s.end () && *i == '=')
    {
      i++;
      if (i == s.end ())
        throw string ("from_san: failed parsing ") + s;
      promote = to_kind (*i++);
      if (promote == NULL_KIND)
        throw string ("from_san: failed parsing ") + s;
    }

  // Parse check or checkmate notation.
  if (i != s.end () && *i == '+') i++;
  if (i != s.end () && *i == '#') i++;

  // At this point, we should know the piece kind and destination of
  // this move and we may have a rank and/or file disambiguating the
  // origin.
  if (to < 0 || to > 64 || k == NULL_KIND) 
    throw string ("from_san: failed parsing ") + s;

  // We now iterate through the legal moves at this position looking
  // for the one we've just read.
  Move m (0, 0, NULL_KIND, 0);
  Move_Vector moves (*this);
  for (int i = 0; i < moves.count; i++)
    {
      // Test whether this move is a candidate for the parsed move.
      if (k == moves[i].get_kind (*this) && to == moves[i].to)
        {
          if (dis_file == -1 && dis_rank == -1) 
            m = moves[i];
          
          if (dis_file != -1 && dis_rank == -1 &&
              idx_to_file (moves[i].from) == dis_file)
            m = moves[i];
          
          if (dis_file == -1 && dis_rank != -1 &&
              idx_to_rank (moves[i].from) == dis_rank)
            m = moves[i];
          
          if (dis_file != -1 && dis_rank != -1 && 
              idx_to_file (moves[i].from) == dis_file &&
              idx_to_rank (moves[i].from) == dis_rank)
            m =  moves[i];
          
          if (!m.is_null ()) break;
        }
    }

  m.promote = promote;

  // Sanity check the move we've constructed.
  if (m.is_null () || 
      m.promote != promote || 
      (m.capture (*this) == NULL_KIND && is_capture))
    throw string ("from_san: failed parsing ") + s;
  
  return m;
}

// Construct a board object from a Forsyth-Edwards Notation position
// string.  Note that only the first six tokens in toks are
// examined. If EPD is true then we are parsing an EPD command and the
// last two fields, clock and half clock, are not expected.
Board
Board::from_fen (const string_vector &toks, bool EPD) {

  ///////////////////////////////////////////////////////////////////////
  // Forsyth-Edwards Notation:                                         //
  //                                                                   //
  // Example: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 //
  //                                                                   //
  // Specification from Wikipedia.                                     //
  // http://en.wikipedia.org/wiki/Forsyth-Edwards_Notation             //
  //                                                                   //
  ///////////////////////////////////////////////////////////////////////

  Board b;
  Board::common_init(b);
  
  // A FEN record contains six fields. The separator between fields is a
  // space. The fields are:
  
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
  if (!toks.size () >= 1) return b;

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
  if (!toks.size () >= 2) return b;
  b.set_color (tolower (toks[1][0]) == 'w' ? WHITE : BLACK);

  // 3. Castling availability. If neither side can castle, this is
  // "-". Otherwise, this has one or more letters: "K" (White can
  // castle kingside), "Q" (White can castle queenside), "k" (Black
  // can castle kingside), and/or "q" (Black can castle queenside).
  b.set_castling_right (W_QUEEN_SIDE, false);
  b.set_castling_right (W_KING_SIDE, false);
  b.set_castling_right (B_QUEEN_SIDE, false);
  b.set_castling_right (B_KING_SIDE, false);

  if (!toks.size () >= 3) return b;

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
  if (!toks.size () >= 4) 
    {
      b.set_en_passant (0);
    }
  else
    {
      if (toks[3][0] != '-') 
        b.set_en_passant 
          ((toks[3][0] - 'a') + 8 * ((toks[3][1] - '0') - 1));
    }
  
  // These fields are not expected when we are parsing an EPD command.
  if (!EPD)
    {
      
      // 5. Halfmove clock: This is the number of halfmoves since the
      // last pawn advance or capture. This is used to determine if a
      // draw can be claimed under the fifty-move rule.
      if (toks.size () >= 5) {
        if (is_number (toks[4])) b.half_move_clock = to_int (toks[4]);
      } else {
        b.half_move_clock = 0;
      }

      // 6. Fullmove number: The number of the full move. It starts at
      // 1 and is incremented after Black's move.
      if (toks.size () >= 6) {
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
  s << " " << (to_move () == WHITE ? 'w' : 'b');

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

// Return an ASCII representation of this position.
string
Board::to_ascii () const {
  ostringstream s;

  // Precede board diagram with status, formated as in FEN strings.
  s << (to_move () == WHITE ? 'w' : 'b') << " ";

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

// Initialize a board from an ascii art representation of a board
// string.
Board
Board::from_ascii (const string &str) {
  Board b;

  Board::common_init (b);
  assert (0);
  return b;
}


