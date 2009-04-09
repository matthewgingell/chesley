////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  session.hpp                                                               //
//                                                                            //
//  This file provides the Session object, representing a Chesley             //
//  session, either over the xboard protocal, UCI (Universal Chess            //
//  Interface), or interactive mode.                                          //
//                                                                            //
//  Matthew Gingell                                                           //
//  gingell@adacore.com                                                       //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _Session_
#define _Session_

#include <cstdio>
#include <unistd.h>
#include "chesley.hpp"

enum Status { GAME_IN_PROGRESS, GAME_WIN_WHITE, GAME_WIN_BLACK, GAME_DRAW };
enum UI_Mode { INTERACTIVE, BATCH };
enum Protocol { NATIVE, UCI, XBOARD };

std::ostream & operator<< (std::ostream &os, Status s);

struct Session {

  ///////////////////////////////
  // Static session variables. //
  ///////////////////////////////

  // State of the chess board.
  static Board board;

  // Color the engine is playing.
  static Color our_color;

  // Is the opponent a computer?
  static bool op_is_computer;

  ///////////////////
  // Engine state. //
  ///////////////////

  // Is session running or paused?
  static bool running;

  // Time in milliseconds since the epoch at which to interrupt an
  // ongoing search, or zero if no timeout is set.
  static uint64 timeout;

  // Command prompt;
  static const char *prompt;

  ///////////////////
  // Constructors. //
  ///////////////////

  // This class is intended to be used as a singleton.
  Session () { assert (0); }

  static void init_session ();

  //////////////
  // Methods. //
  //////////////

  // Enter the command loop, which will not return until the session
  // is over.
  static void cmd_loop ();

  /////////////////////
  // Interface mode. //
  /////////////////////

  // User interface mode.
  static UI_Mode ui_mode;

  // Protocol mode.
  static Protocol protocol;

  ///////////////////
  // Search state. //
  ///////////////////

  static Search_Engine se;

  /////////////////
  // Statistics. //
  /////////////////

  static void collect_new_game ();
  static void collect_statistics ();
  static void collect_game_over ();

  static int counts_this_game [6][64];
  static int counts_all_games [6][64];
  static int num_games;

  ////////////////////////////////////////
  // I/O handling and command dispatch. //
  ////////////////////////////////////////

  static FILE *in, *out;
  static bool tty;

  // Handle an alarm signal.
  static void handle_alarm (int sig);

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

  ///////////////////
  // Flow control. //
  ///////////////////

  // Control is turned over to the engine to do as it wishes until
  // either the timeout expires, there is input pending from the user,
  // or the interface needs to block and wait for input from the user.
  static void work ();

  // Determine the current status of this game.
  static Status get_status ();

  // Set session state to reflect the game has ended.
  static void handle_end_of_game (Status s);

  // Find a move to play.
  static Move find_a_move ();

  ///////////////
  // Commands. //
  ///////////////

  // Process a string in Extended Position Notation.
  static bool epd (const string_vector &tokens);

  /////////////////////////
  // Debugging commands. //
  /////////////////////////

  // Generate a benchmark.
  static bool bench (const string_vector &tokens);

  // Compute possible moves from a position.
  static bool perft (const string_vector &tokens);

  // Play a game with engine taking both sides.
  static bool play_self (const string_vector &tokens);

  // Check that hash keys are correctly generated to depth 'd'.
  static void test_hashing (int d);
};

#endif /* _Session_ */
