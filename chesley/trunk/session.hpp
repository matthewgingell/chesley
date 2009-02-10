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
  
  // Indicate to clients that control should be returned to the
  // command loop as soon as possible.
  static bool halt;

  // State of play.
  static Board board;

  // Search engine begin used.
  static Search_Engine se;

private:

  /*******************/
  /* Interface mode. */
  /*******************/

  enum Mode { INTERACTIVE, UCI, XBOARD };
  static Mode mode;

  /**************/
  /* I/O status */
  /**************/

  static FILE *in;
  static FILE *out;
  static const char *prompt;
  static bool tty;
  static const int buf_max;
  
  static void handle_interrupt (int sig);
  
  // Command handling.
  static bool execute (char *line);

  // XBoard specific command handling.
  static bool xbd_execute (char *line);

  // Set xboard protocol mode.
  static bool set_xboard_mode (const string_vector &tokens);

  /********************/
  /* Generic commands */
  /********************/

  // Generate a benchmark.
  static bool bench (const string_vector &tokens); 

  // Compute possible moves from a position.
  static bool perft (const string_vector &tokens);

  // Play a game against its self.
  static bool play_self (const string_vector &tokens);

  



};

#endif /* _Session_ */
