/* 
   This file provides the Session object, representing a Chesley's session
   over the Universal Chess Interface (UCI).

   Matthew Gingell
   gingell@adacore.com
*/

#include <cstring>
#include <cstdio>
#include <signal.h>
#include <string>
#include <sys/time.h>

#include "chesley.hpp"

// Putting cctype here, after include of <iostream>, works around a
// bug in some version of the g++ library.

#include <cctype>

using namespace std;

#include <iostream>
#include <fstream>

/*********************/
/* Session variables */
/*********************/

Session::Mode Session::mode;
bool          Session::halt;
FILE         *Session::in;
FILE         *Session::out;
bool          Session::tty;

const char *Session::prompt = "> ";

ofstream log;

/*********/
/* State */
/*********/

Board Session::board;
Status Session::status;
Search_Engine Session::se;
Color Session::our_color;
bool Session::op_is_computer;

/******************************/
/* Initialize the environment */
/******************************/

void
Session::init_session () {

  // Setup I/O.
  in = stdin;
  out = stdout;
  setvbuf (in, NULL, _IONBF, 0);
  setvbuf (out, NULL, _IONBF, 0);
  tty = isatty (fileno (in));

  // Set mode.
  mode = INTERACTIVE;

  // Set initial game state.
  our_color = BLACK;
  board = Board :: startpos ();
  status = GAME_IN_PROGRESS;
  se = Search_Engine (6);
  op_is_computer = false;

  // Setup periodic alarm.
  halt = 0;
  struct itimerval timer;
  timer.it_value.tv_sec = 1;
  timer.it_value.tv_usec = 0;
  timer.it_interval.tv_sec = 1;
  timer.it_interval.tv_usec = 0;
  setitimer(ITIMER_REAL, &timer, NULL);
  signal (SIGALRM, handle_interrupt);

  return;
}

// Set halt when input is pending.
void 
Session::handle_interrupt (int sig) {

  //  cerr << "got SIGALRM" << endl;

  if (fdready (fileno (in)))
    {
      halt = true;
    }
  else
    {
      halt = false;
    }
}

// Write the command prompt.
void Session::write_prompt ()
{
  if (prompt) {
    fprintf (out, "%s", prompt);
    fflush (out);
  }
}

/***************************/
/* Top level command loop. */
/***************************/

void
Session::cmd_loop () 
{
  fprintf (out, PROLOGUE);
  write_prompt ();
 
 while (true)
    {
      char *line = get_line (in);
      cerr << "Chesley got \"" << line << "\"" << endl;
      
      // Break out of command loop at end of input.
      if (!line || !execute (line))
	{
	  if (line) 
	    {
	      free (line);
	    }
	  break;
	}
      
      write_prompt ();

      // Loop until input is ready.
      while (!fdready (fileno (in)) && status == GAME_IN_PROGRESS) 
	{
	  if (board.flags.to_move == our_color)
	    {
	      // Send a move to the client.
	      try 
		{
		  Move best = se.choose_move (board);
		  board.apply (best);
		  fprintf (out, "move %s\n", board.to_calg (best).c_str ());
		}
	      catch (Game_Over s)
		{
		  handle_end_of_game (s.status);
		  break;
		}
	      
	      // Don't busy wait for input.
	      usleep (10000);
	    }
	}
    }
}

/*****************************************/
/* Report and clean up when a game ends. */
/*****************************************/

 void 
 Session::handle_end_of_game (Status s)
 {
   status = s;

   switch (s)
     {
     case GAME_WIN_WHITE:
       fprintf (out, "RESULT 1-0\n");
       break;
       
     case GAME_WIN_BLACK:
       fprintf (out, "RESULT 0-1\n");
       break;
       
     case GAME_DRAW:
       fprintf (out, "RESULT 1/2-1/2\n");
       break;
       
     default:
       assert (0);
     }
}


/*************************************/
/* Parse and execute a command line. */
/*************************************/

bool 
Session::execute (char *line) {

  /*******************************************************************/
  /* Give the mode-specific handler a chance to process this command */
  /* before handing it to the handler below.                         */
  /*******************************************************************/

  if (mode == XBOARD)
    {
      if (!xbd_execute (line))
	{
	  return false;
	}
    }

  string_vector tokens = tokenize (line);
  int count = tokens.size ();

  if (count > 0)
    {
      string token = downcase (tokens[0]);

      /**********************/
      /* Benchmark command. */
      /**********************/

      if (token == "bench") 
	{
	  return bench (tokens);
	}

      /**************************************/
      /* Perft position generation command. */
      /**************************************/
      
      if (token == "perft") 
	{
	  return perft (tokens);
	}

      /**********************************/
      /* The play against self command. */
      /**********************************/

      if (token == "playself")
	{
	  return play_self (tokens);
	}

      /*************************/
      /* Perform various tests */
      /*************************/
      if (token == "test")
	{
	  board = Board::from_fen ("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");

	  // Expect 48.
	  cerr << board.perft (1) << endl; // Get 48
	  
	  // Expect 2039.
	  cerr << board.perft (2) << endl; // Get 2038????????

	  abort ();

	  // Expect 97862.
	  cerr << board.perft (3) << endl;

	  // Expect 4085603
	  cerr << board.perft (4) << endl;
	  
	  // Expect 193690690
	  cerr << board.perft (5) << endl;
	}

      /****************/
      /* Quit command */
      /***************/

      if (token == "quit")  
	{
	  return false;
	}

      /**********************/
      /* Enter xboard mode. */
      /**********************/

      if (token == "xboard") 
	{
	  return set_xboard_mode (tokens);
	}
    }
  
  // Ignore unrecognized commands.
  return true;
}
