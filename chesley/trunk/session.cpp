/*
   This file provides the Session object, representing a Chesley's
   session over the Universal Chess Interface (UCI).

   Matthew Gingell
   gingell@adacore.com
*/

#include <cstring>
#include <cstdio>
#include <signal.h>
#include <string>
#include <sys/time.h>

#include "chesley.hpp"

// For now we use a hardcoded timeout in milliseconds.
const int TIME_OUT = 5 * 1000;

// Putting cctype here, after include of <iostream>, works around a
// bug in some version of the g++ library.

#include <cctype>

using namespace std;

#include <iostream>
#include <fstream>

/*********************/
/* Session variables */
/*********************/

Session::UI   Session::ui;
FILE         *Session::in;
FILE         *Session::out;
bool          Session::tty;
bool          Session::running;
uint64        Session::timeout;

const char *Session::prompt = "> ";

/*********/
/* State */
/*********/

Board Session::board;
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

  // Set ui.
  if (tty)
    {
      ui = INTERACTIVE;
    }
  else
    {
      ui = BATCH;
      prompt = (char *) 0;
    }

  // Set initial game state.
  our_color = BLACK;
  board = Board :: startpos ();
  se = Search_Engine (100);
  op_is_computer = false;
  running = false;

  // Setup periodic alarm.
  struct itimerval timer;
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 10000;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 10000;
  setitimer(ITIMER_REAL, &timer, NULL);
  signal (SIGALRM, handle_alarm);

  return;
}

/*******************************************************************/
/* Catch an alarm periodically. This is used for asynchronous I/O. */
/* handling.                                                       */
/*******************************************************************/

void
Session::handle_alarm (int sig) {
  // Interrupt an ongoing search when time has elapsed.
  if (mclock () > timeout || fdready (fileno (in)))
      se.interrupt_search = true;
}

// Write the command prompt.
void Session::write_prompt ()
{
  if (prompt && (ui == INTERACTIVE))
    fprintf (out, "%s", prompt);
}

/*************/
/* Searching */
/*************/

Move Session::get_move () {
  timeout = mclock () + TIME_OUT;
  return se.choose_move (board);
}

/***************************/
/* Top level command loop. */
/***************************/

void
Session::cmd_loop ()
{
  bool done = false;

  if (ui == INTERACTIVE)
    {
      fprintf (out, PROLOGUE);
    }

  write_prompt ();
  while (true)
    {
      // Block for input.
      char *line = get_line (in);

      if ((!line) || (!execute (line))) done = true;
      if (line) free (line);
      if (done) break;

      write_prompt ();

      // Loop and do some work until input is ready.
      while (!fdready (fileno (in)))
	{
	  work ();
	  usleep (100000);
	}
    }
}

/****************/
/* Flow control */
/****************/

// Control is turned over to us, either to make a move, ponder,
// analyze, garbage collect, etc.
void
Session::work ()
{
  // If the game isn't over yet, generate a move and send it to
  // the client.
  if (running
      && board.get_status () == GAME_IN_PROGRESS
      && board.flags.to_move == our_color)
    {
      Move best = get_move ();

      if (board.apply (best))
	{
	  fprintf (out, "move %s\n", board.to_calg (best).c_str ());
	}

      Status s = board.get_status ();
      if (s != GAME_IN_PROGRESS)
	{
	  handle_end_of_game (s);
	}
    }

  return;
}

// Report and clean up when a game ends.
void
Session::handle_end_of_game (Status s) {

  switch (s)
    {
    case GAME_WIN_WHITE:
      fprintf (out, "result 1-0\n");
      break;

    case GAME_WIN_BLACK:
      fprintf (out, "result 0-1\n");
      break;

    case GAME_DRAW:
      fprintf (out, "result 1/2-1/2\n");
      break;

    default:
      assert (0);
    }

  running = false;
}

/*************************************/
/* Parse and execute a command line. */
/*************************************/

bool
Session::execute (char *line) {

  /*******************************************************************/
  /* Give the ui-specific handler a chance to process this command   */
  /* before handing it to the handler below.                         */
  /*******************************************************************/

  if (ui == XBOARD)
    {
      if (!xbd_execute (line))
	{
	  return false;
	}
    }

  // Apply a debugging command.
  if (!debug_execute (line))
    {
      return false;
    }

  string_vector tokens = tokenize (line);
  int count = tokens.size ();

  // Execute a standard command.
  if (count > 0)
    {
      string token = downcase (tokens[0]);

      if (token == "apply" && count > 1)
	{
	  running = false;
	  Move m = board.from_calg (tokens[1]);
	  board.apply (m);
	}

      if (token == "hash")
	{
	  cerr << board.hash << endl;
	}

      if (token == "eval")
	{
	  cerr << eval (board) << endl;
	}

      /**********************/
      /* Benchmark command. */
      /**********************/

      if (token == "bench")
	{
	  return bench (tokens);
	}

      /****************/
      /* Disp command */
      /****************/

      if (token == "disp")
	{
	  fprintf (out, "%s\n", (board.to_ascii ()).c_str ());
	  return true;
	}

      /******************/
      /* Div command */
      /*****************/

      if (token == "div")
	{
	  if (tokens.size () == 2)
	    {
	      board.divide (to_int (tokens[1]));
	      return true;
	    }
	}

      /**************************************/
      /* FEN command to output board state. */
      /**************************************/

      if (token == "fen")
	{
	  fprintf (out, "%s\n", (board.to_fen ()).c_str ());
	  return true;
	}

      /**************************************/
      /* EPD command execute an EPD string. */
      /**************************************/

      if (token == "epd")
	{
	  epd (tokens);
	  return true;
	}

      /*******************************************************/
      /* Force command to prevent computer generating moves. */
      /*******************************************************/

      if (token == "force")
	{
	  running = false;
	  return true;
	}

      /******************/
      /* Moves command. */
      /******************/

      if (token == "moves")
	{
	  Move_Vector moves (board);
	  cerr << moves << endl;
	}

      /********************/
      /* Attacks command. */
      /********************/

      if (token == "attacks")
	{
	  print_board (board.attack_set (invert_color (board.flags.to_move)));
	}

      /**************************************/
      /* Perft position generation command. */
      /**************************************/

      if (token == "perft")
	{
	  if (tokens.size () >= 2)
	    {
	      perft (tokens);
	      // fprintf (out, "%lli\n", board.perft (to_int (tokens[1])));
	      return true;
	    }
	}

      /**********************************/
      /* The play against self command. */
      /**********************************/

      if (token == "playself")
	{
	  return play_self (tokens);
	}

      /************************/
      /* setboard FEN command */
      /************************/

      if (token == "setboard")
	{
	  board = Board::from_fen (rest (tokens), false);
	  return true;
	}

      /****************/
      /* Quit command */
      /***************/

      if (token == "quit")
	{
	  return false;
	}

      /********************/
      /* Enter xboard ui. */
      /********************/

      if (token == "xboard")
	{
	  return set_xboard_mode (tokens);
	}
    }

  // Warn about unrecognized commands in interactive mode.
  if (ui == INTERACTIVE)
    {
      fprintf (out, "Unrecognized command.\n");
    }

  return true;
}
