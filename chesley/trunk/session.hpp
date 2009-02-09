/* 
   This file provides the Session object, representing a Chesley
   session, either over the Xboard protocal, UCI (Universal Chess
   Interface), or interactive mode.

   Matthew Gingell
   gingell@adacore.com
*/

#ifndef _Session_
#define _Session_

using namespace std;

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

  /**************/
  /* I/O status */
  /**************/

  static FILE *in;
  static FILE *out;
  static const char *prompt;
  static bool tty;
  static const int buf_max;
  
  static void handle_interrupt (int sig);
  static bool execute (char *line);

  /************/
  /* Commands */
  /************/

  // Generate a benchmark.
  static bool bench (const string_vector &tokens); 

  // Compute possible moves from a position.
  static bool perft (const string_vector &tokens);
};

#endif /* _Session_ */
