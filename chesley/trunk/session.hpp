/*
   This file provides the Session object, representing a Chesley
   session, either over the xboard protocal, UCI (Universal Chess
   Interface), or interactive mode.

   Matthew Gingell
   gingell@adacore.com
*/

#ifndef _Session_
#define _Session_

#include <stdio.h>
#include <unistd.h>
#include "board.hpp"
#include "search.hpp"
#include "util.hpp"

struct Session {

  /****************/
  /* Constructors */
  /****************/

  // This class is intended to be used as a singleton.
  Session () { assert (0); }

  static void init_session ();

  /************/
  /* Methods. */
  /************/

  // Enter the command loop. Control does not return to the caller
  // until the user session is over.
  static void cmd_loop ();

  /***************/
  /* Game state. */
  /***************/

  // State of play.
  static Board board;

  // Color the engine is playing.
  static Color our_color;

  // Search engine begin used.
  static Search_Engine se;

  // Is the opponent a computer?
  static bool op_is_computer;

private:

  /*******************/
  /* Interface mode. */
  /*******************/

  enum UI { INTERACTIVE, BATCH, UCI, XBOARD };
  static UI ui;

  /****************/
  /* I/O handling */
  /****************/

  static FILE *in;
  static FILE *out;
  static const char *prompt;
  static bool tty;

  // Handle an alarm signal.
  static void handle_alarm (int sig);

  // Write the command prompt.
  static void write_prompt ();

  // Command handling.
  static bool execute (char *line);

  // XBoard specific command handling.
  static bool xbd_execute (char *line);

  // Execute a debugging command.
  static bool debug_execute (char *line);

  // Set xboard protocol mode.
  static bool set_xboard_mode (const string_vector &tokens);

  /****************/
  /* Flow control */
  /****************/

  // Is session running or paused?
  static bool running;

  // Control is turned over to us, either to make a move, ponder,
  // analyze, etc.
  static void work ();

  // Time in milliseconds since the epoch to interrupt an ongoing
  // search.
  static uint64 timeout;

  // Initiate a timed search.
  static Move get_move ();

  // Report and clean up when a game ends.
  static void handle_end_of_game (Status s);

  /************/
  /* Commands */
  /************/

  // Process a string in Extended Position Notation. This can include
  // tests, etc.
  static bool epd (const string_vector &tokens);

  /**********************/
  /* Debugging commands */
  /**********************/

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
