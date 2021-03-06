////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// session.hpp                                                                //
//                                                                            //
// This file provides the Session object, representing a Chesley              //
// session, either over the xboard protocol, UCI (Universal Chess             //
// Interface), or interactive mode.                                           //
//                                                                            //
// Copyright Matthew Gingell <matthewgingell@gmail.com>, 2009-2015.           //
// Chesley the Chess Engine! is free software distributed under the terms of  //
// the GNU Public License.                                                    //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _Session_
#define _Session_

#include <cstdio>

#include "chesley.hpp"

enum Status   { GAME_IN_PROGRESS, GAME_WIN_WHITE, GAME_WIN_BLACK, GAME_DRAW };
enum UI_Mode  { INTERACTIVE, BATCH };
enum Protocol { NATIVE, UCI, XBOARD };

std::ostream & operator<< (std::ostream &os, Status s);

////////////////
// Exceptions //
////////////////

const int SEARCH_INTERRUPTED = 1;

struct Session {

  //////////////////////////////
  // Static session variables //
  //////////////////////////////

  // State of the chess board.
  static Board board;

  // The principle variation computed for this board, or otherwise an
  // empty list.
  static Move_Vector pv;
  // Color that Chesley is playing.
  static Color our_color;

  // Is the opponent a computer?
  static bool op_is_computer;

  //////////////////
  // Engine state //
  //////////////////

  // Is session running or stopped?
  static bool running;

  // Should we interupt the search if we have pending input?
  static bool interrupt_on_io;

  // Is pondering enabled?
  static bool ponder_enabled;

  // Command prompt.
  static const char *prompt;

  //////////////////
  // Constructors //
  //////////////////

  // This class is intended to be used as a singleton.
  Session () { assert (0); }

  // Initialize the main command loop.
  static void init_session ();

  /////////////
  // Methods //
  /////////////

  // Enter the command loop, which will not return until the session
  // is over.
  static void cmd_loop ();

  ////////////////////
  // Interface mode //
  ////////////////////

  // User interface mode.
  static UI_Mode ui_mode;

  // Protocol mode.
  static Protocol protocol;

  //////////////////
  // Search state //
  //////////////////

  static Search_Engine se;

  ///////////////////////////////////////
  // I/O handling and command dispatch //
  ///////////////////////////////////////

  static FILE *in, *out;
  static bool tty;

  // Write the command prompt.
  static void write_prompt ();

  // Execute a command line.
  static bool execute (char *line);

  // Execute an xboard specific command line.
  static bool xbd_execute (char *line);

  // Execute a debugging command.
  static bool debug_execute (char *line);

  // Set xboard protocol mode.
  static bool set_xboard_mode (const string_vector &tokens);

  //////////////////
  // Flow control //
  //////////////////

  // Control is turned over to the engine to do as it wishes until
  // either the timeout expires, there is input pending from the user,
  // or the interface needs to block and wait for input from the user.
  static void work ();

  // Determine the current status of this game.
  static Status get_status (Board &b);

  // Set session state to reflect the game has ended.
  static void handle_end_of_game (Status s);

  // Find a move to play.
  static Move find_a_move ();

  // Ponder the next move in the principle variation.
  static void do_ponder ();

  // This function must be called periodically to implement timeouts.
  static void poll ();

  //////////////
  // Commands //
  //////////////

  // ??? Move implementations to commands.cpp?

  // Process a string in Extended Position Notation.
  static bool epd (const string_vector &tokens);

  // Set up time controls from level command.
  static bool level (const string_vector &tokens);

  // Display the current time controls.
  static bool display_time_controls (const string_vector &tokens);

  // Display the a help message.
  static bool display_help (const string_vector &tokens);

  ////////////////////////
  // Debugging commands //
  ////////////////////////

  // Generate a benchmark.
  static bool bench (const string_vector &tokens);

  // Play a game with engine taking both sides.
  static bool play_self (const string_vector &tokens);

  // Dump pawn struct vectors to a file.
  static bool dump_pawns (const string_vector &tokens);

  // Check that hash keys are correctly generated to depth 'd'.
  static void test_hashing (int d);
};

#endif // _Session_
