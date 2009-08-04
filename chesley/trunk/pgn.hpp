///////////////////////////////////////////////////////////////////////////////
//                                                                            //
// pgn.hpp                                                                    //
//                                                                            //
// Utilities for working with Portable Game Notation files.                   //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley             //
// the Chess Engine! is free software distributed under the terms             //
// of the GNU Public License.                                                 //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _PGN_
#define _PGN_

#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include "board.hpp"
#include "move.hpp"

struct Game {
  // Either black white or null.
  Color winner; 
  std::vector <Move> moves;
  std::map <std::string, std::string> metadata;
};

struct PGN {

  PGN ();

  // Open a stream from a file.
  void open (const char *filename);

  // Close the stream. 
  void close ();

  // Read a game from the file.
  Game read_game ();

  // Read the meta data at the current offset.
  void read_metadata (Game &g);

  // Read a list of moves from the steam.
  void read_moves (Game &g);

  // Skip a '{ ... }' comment or whitespace.
  void skip_comment_and_whitespace ();

// Skip a '( ... )' in the moves list.
  void skip_recursive_variation ();

  // An internal board against which moves are validated and parsed.
  Board b;
  
  // Data.
  enum { OK, END_OF_FILE, RECOVERABLE_ERROR, FATAL_ERROR } status;
  FILE *fp;
};



#endif // _PGN_
