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

/* Putting cctype here, after include of <iostream>, works around a
   bug in some version of the g++ library. */

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
  se = Search_Engine (6);
  op_is_computer = false;

  // Handle interrupts.
  //  signal (SIGINT, handle_interrupt);


  // DEBUG

  log.open ("LOG");

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

// Write the command prompt.
void Session::write_prompt ()
{
  if (prompt) {
    fprintf (out, "%s", prompt);
    fflush (out);
  }
}

/*************************/
/* Session command loop. */
/*************************/

void
Session::cmd_loop () 
{
  char *line;

  fprintf (out, PROLOGUE);
  write_prompt ();
  while (true)
    {
      line = get_line (in);

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

      while (!fdready (fileno (in))) 
	{
	  // Control is turned over to us until the client provides
	  // input.
	  if (board.flags.to_move == our_color)
	    {
	      Move best = se.choose_move (board);

	      log << board << endl;
	      log << flush;

	      fprintf (out, "move %s\n",  board.to_calg (best).c_str ());
	      // cerr << board << endl;
	      // cerr << "Sent move: " << best << endl;
	      board.apply (best);
	    }

	  usleep (100000);
	}
    }
}

/*************************************/
/* Parse and execute a command line. */
/*************************************/

bool 
Session::execute (char *line) {

  /*******************************************************************/
  /* Give the mode-specific handler a chance to process this command */
  /* before handing it to the generic list below.                    */
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


	  board = Board::from_fen ("2k2b1r/ppp1npp1/2n3p1/8/5PPq/4P2P/PP1r2K1/R6R w - - 0 20");
	  Move best = se.choose_move (board);

	  cerr << board << endl;
	  cerr << best << endl;

#if 0
	  board = Board::from_fen ("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");

	  // Expect 48.
	  cerr << board.perft (1) << endl;
	  
	  // Expect 2039.
	  cerr << board.perft (2) << endl;

	  // Expect 97862.
	  cerr << board.perft (3) << endl;

	  // Expect 4085603
	  cerr << board.perft (4) << endl;
	  
	  // Expect 193690690
	  cerr << board.perft (5) << endl;

	  cerr << endl;
#endif 

#if 0
	  board = Board::from_fen ("8/3K4/2p5/p2b2r1/5k2/8/8/1q6 b - - 1 67");


	  cerr << board << endl;

	  // Expect 50
	  cerr << board.perft (1) << endl;

	  // Expect 279
	  cerr << board.perft (2) << endl;

	  cerr << endl;

	  abort ();
	  
	  /*
	    8/7p/p5pb/4k3/P1pPn3/8/P5PP/1rB2RK1 b - d3 0 28
	    Perft(6) = 38633283
	    
	    rnbqkb1r/ppppp1pp/7n/4Pp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3
	    Perft(5) = 11139762
	    
	    8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -
	    Perft(6) = 11030083
	    Perft(7) = 178633661
	  */
#endif


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

// Execute a command in xboard mode. The specification for this
// protocol is http://tim-mann.org/xboard/engine-intf.html.
bool 
Session::xbd_execute (char *line) {
  string_vector tokens = tokenize (line);
  int count = tokens.size ();

  if (tokens.size () > 0) 
    {
      string token = downcase (tokens[0]);

      /**********************/
      /* protover N command */
      /**********************/

      if (token == "protover") 
	{
	  // Send xboard the list of features we support.

	  fprintf (out, "feature ping=1\n");
	  fprintf (out, "feature setboard=1\n");
	  fprintf (out, "feature playother=1\n");
	  fprintf (out, "feature san=0\n");
	  fprintf (out, "feature usermove=1\n");
	  fprintf (out, "feature time=1\n");
	  fprintf (out, "feature draw=1\n");
	  fprintf (out, "feature sigint=0\n");
	  fprintf (out, "feature sigterm=1\n");
	  fprintf (out, "feature reuse=1\n");
	  fprintf (out, "feature analyze=0\n");
	  fprintf (out, "feature myname=\"Chesley The Chess Engine!\"\n");
	  fprintf (out, "feature colors=0\n");
	  fprintf (out, "feature ics=1\n");
	  fprintf (out, "feature name=1\n");
	  fprintf (out, "feature pause=1\n");
	  fprintf (out, "feature done=1\n");
	}

      /**************************************************************/
      /* Assume we are using an up to date version of xboard and do */
      /* not pay attention to replies to the feature command.       */
      /**************************************************************/
         
      if (token == "accepted" || token == "rejected")
	{
	  // ignored.
	}

      /***************/
      /* new command */
      /***************/

      if (token == "new")
	{
	  board = Board :: startpos ();
	}

      /******************************************************/
      /* We do not support any non-standard chess variants. */
      /******************************************************/

      if (token == "variant") 
	{
	  // ignored.
	}

      /****************/
      /* quit command */
      /****************/

      if (token == "quit") 
	{
	  return false;
	}

      /******************/
      /* random command */
      /******************/

      if (token == "random") 
	{
	  // ignored.
	}

      /*****************/
      /* force command */
      /*****************/

      if (token == "force") 
	{
	  our_color = NULL_COLOR;
	}

      /**************/
      /* go command */
      /**************/

      if (token == "go") 
	{
	  our_color = board.flags.to_move;
	  // ignored.
	}

      /*********************/
      /* playother command */
      /*********************/

      if (token == "playother") 
	{
	  our_color = invert_color (board.flags.to_move);
	}

      /*****************/
      /* white command */
      /*****************/

      if (token == "white") 
	{
	  board.flags.to_move = WHITE;
	  our_color = BLACK;
	}

      /*****************/
      /* black command */
      /*****************/

      if (token == "black") 
	{
	  board.flags.to_move = BLACK;
	  our_color = WHITE;
	}

      /******************************/
      /* level MPS BASE INC command */
      /******************************/

      if (token == "level") 
	{
	  // ignored.
	}

      /*******************/
      /* st TIME command */
      /*******************/

      if (token == "st") 
	{
	  // ignored.
	}

      /********************/
      /* sd DEPTH command */
      /********************/

      if (token == "sd") 
	{
	  int d;
	  if (sscanf (token.c_str (), "%i", &d) > 0)
	    {
	      se.max_depth = d;
	    }
	}

      /******************/
      /* time N command */
      /******************/

      if (token == "time") 
	{
	  // ignored.
	}

      /*******************/
      /* otime N command */
      /*******************/

      if (token == "otime") 
	{
	  // ignored.
	}

      /********************/
      /* usermove command */
      /********************/

      if (token == "usermove" && count > 1 && board.is_calg (tokens[1]))
	{
	  Move m = board.from_calg (tokens[1]);	  
	  board.apply (m);
	}

      /**************/
      /* ? command. */
      /**************/

      if (token == "?") 
	{
	  // ignored.
	}

      /*******************/
      /* ping N command. */
      /*******************/

      if (token == "ping") {
	fprintf (out, "pong");
	if (count > 1) 
	  {
	    fprintf (out, " %s", tokens[1].c_str ());
	  }
	fprintf (out, "\n");
      }

      /****************/
      /* draw command */
      /****************/

      if (token == "draw") 
	{
	  // ignored.
	}

      /***********************************/
      /* result RESULT {COMMENT} command */
      /***********************************/

      if (token == "result") 
	{
	  // ignored.
	}

      /************************/
      /* setboard FEN command */
      /************************/

      if (token == "setboard") 
	{
	  // ignored.
	}

      /****************/
      /* edit command */
      /****************/

      if (token == "edit") 
	{
	  // ignored.
	}

      /****************/
      /* hint command */
      /****************/

      if (token == "hint") 
	{
	  // ignored.
	}

      /**************/
      /* bk command */
      /**************/

      if (token == "bk") 
	{
	  // ignored.
	}

      /****************/
      /* undo command */
      /****************/

      if (token == "undo") 
	{
	  // ignored.
	}

      /******************/
      /* remove command */
      /******************/

      if (token == "remove") 
	{
	  // ignored.
	}

      /****************/
      /* hard command */
      /****************/

      if (token == "hard") 
	{
	  // ignored.
	}

      /****************/
      /* easy command */
      /****************/

      if (token == "easy") 
	{
	  // ignored.
	}

      /****************/
      /* post command */
      /****************/

      if (token == "post") 
	{
	  // ignored.
	}

      /******************/
      /* nopost command */
      /******************/

      if (token == "nopost") 
	{
	  // ignored.
	}

      /*******************/
      /* analyze command */
      /*******************/

      if (token == "analyse") 
	{
	  // ignored.
	}

      /******************/
      /* name X command */
      /******************/

      if (token == "name") 
	{
	  // ignored.
	}

      /******************/
      /* rating command */
      /******************/

      if (token == "rating") 
	{
	  // ignored.
	}

      /************************/
      /* ics HOSTNAME command */
      /************************/

      if (token == "ics") 
	{
	  // ignored.
	}

      /********************/
      /* computer command */
      /********************/

      if (token == "computer") 
	{
	  op_is_computer = true;
	}

      /*****************/
      /* pause command */
      /*****************/

      if (token == "pause") 
	{
	  // ignored.
	}

      /******************/
      /* resume command */
      /******************/

      if (token == "resume") 
	{
	  // ignored.
	}
    }

  return true;
}

// Set xboard protocol mode.
bool 
Session::set_xboard_mode (const string_vector &tokens) {
  // Set mode.
  mode = XBOARD;

  // Prompts confuse XBoard so switch them off.
  prompt = NULL;

  // Set chatting for ICS.
  fprintf (out, "tellicsnoalias set 1 %s v%s\n", ENGINE_ID_STR, VERSION_STR);
  fprintf (out, "tellicsnoalias kibitz Chesley! v%s says hello!\n", VERSION_STR);

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

  catch (Game_Over e)
    {
      cout << e << endl;
    }

  return true;
}
