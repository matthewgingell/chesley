/* 
   This file provides the Session object, representing a Chesley's session
   over the Universal Chess Interface (UCI).

   Matthew Gingell
   gingell@adacore.com
*/

#include <cstring>
#include <cstdio>
#include <readline/readline.h>
#include <signal.h>
#include <string>
#include <sys/time.h>

#include "chesley.hpp"

/* Putting cctype here, after include of <iostream> works around a bug
   in some version of the g++ library. */

#include <cctype>

using namespace std;

/*********************/
/* Session variables */
/*********************/

bool  Session::halt;
FILE *Session::in;
FILE *Session::out;
bool  Session::tty;

const char *Session::prompt = "> ";

/*********/
/* State */
/*********/

Board Session::board;
Search_Engine Session::se;

/******************************/
/* Initialize the environment */
/******************************/

void
Session::init_session () {
  // Setup streams.
  in = stdin;
  out = stdout;
  tty = isatty (fileno (in));

  // Setup state.
  board = Board :: startpos ();
  se = Search_Engine (6);

  signal (SIGINT, handle_interrupt);

#if 0
  halt = 0;
  struct itimerval timer;
  timer.it_value.tv_sec = 1;
  timer.it_value.tv_usec = 0;
  timer.it_interval.tv_sec = 1;
  timer.it_interval.tv_usec = 0;
  setitimer(ITIMER_REAL, &timer, NULL);
  signal (SIGALRM, handle_interrupt);
#endif
}

// Set the halt flag depending on whether there is input waiting.
void 
Session::handle_interrupt (int sig) {
#if 0
  if (fdready (fileno (in)))
    {
      halt = true;
    }
  else
    {
      halt = false;
    }
#endif

  cerr << "Caught SIGINT" << endl;
}

/*************************************/
/* Session command interpreter loop. */
/*************************************/

void
Session::cmd_loop () 
{
  fprintf (out, PROLOGUE);
  while (true)
    {
      char *line = readline (Session::prompt);
      if (!line || !execute (line)) break;
      free (line);
    }
}

/*************************************/
/* Parse and execute a command line. */
/*************************************/

bool 
Session::execute (char *line) {
  string_vector tokens = tokenize (line);
  int count = tokens.size ();

  cerr << "Chesley got: " << line << endl;

  if (tokens.size () > 0)
    {
      string token = downcase (tokens[0]);

      if (token == "bench") 
	{
	  return bench (tokens);
	}
      
      if (token == "perft") {
	return perft (tokens);
      }

      if (token == "ping") {
	fprintf (out, "pong");
	if (count > 1) 
	  {
	    fprintf (out, " %s", tokens[1].c_str ());
	  }
	fprintf (out, "\n");
      }

      if (token == "quit")  
	{
	  return false;
	}

      if (token == "xboard") 
	{
	  fprintf (out, "tellicsnoalias set 1 %s v%s\n", ENGINE_ID_STR, VERSION_STR);
	  fprintf (out, "tellicsnoalias kibitz Chesley! v%s says hello!\n", VERSION_STR);
	  prompt = NULL;
	}

      if (token == "protover") 
	{
	  fprintf (out, "feature\n");
	}

      if (board.is_calg (token))
	{
	  Move m = board.from_calg (token);	  
	  board.apply (m);
	  Move best = se.choose_move (board);
	  fprintf (out, "move %s\n",  board.to_calg (best).c_str ());
	  fflush (out);
	  cerr << "Sent move: " << best << endl;
	  board.apply (best);
	}
      if (token == "playself")
	{
	  return play_self (tokens);
	}

    }
  
  return true;
}

// Generate a benchmark.
bool 
Session::bench (const string_vector &tokens) {
  int depth = 6;
  board = Board::startpos ();

  if (tokens.size () > 1 && is_number (tokens[1]))
    {
      depth = to_int (tokens[1]);
    }

  fprintf (out, "Computing best move to ply %i...\n", depth);

  double start_time = user_time (); 
  se.score_count = 0;
  se.max_depth = depth;
  se.choose_move (board);
  double elapsed = user_time () - start_time;

  fprintf (out, "%lli calls to score.\n", se.score_count); 
  fprintf (out, "%.2f seconds elapsed.\n", elapsed);
  fprintf (out, "%.2f calls/second.\n", ((double) se.score_count) / elapsed);

  return true;
}

// Compute possible moves from a position.
bool 
Session::perft (const string_vector &tokens)
{
  int depth = 1;
  double start, elapsed;
  uint64 count;

  board = Board::startpos ();

  if (tokens.size () > 1 && is_number (tokens[1]))
    {
      depth = to_int (tokens[1]);
    }

  fprintf (out, "Computing perft to depth %i...\n", depth);

  start = user_time (); 
  count = board.perft (depth);
  elapsed = user_time () - start;
  fprintf (out, "moves = %lli\n", count); 
  fprintf (out, "%.2f seconds elapsed.\n", elapsed);
  fprintf (out, "%.2f moves/second.\n", ((double) count) / elapsed);  

  return true;
}

// Play a game against its self.
bool 
Session::play_self (const string_vector &tokens) 
{
  board = Board::startpos ();

  try {
    while (1)
      {
	cerr << board << endl;
	Move m = se.choose_move (board);
	board.apply (m);
      }
  }

  catch (...)
    {
      return true;
    }
}
