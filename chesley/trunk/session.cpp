////////////////////////////////////////////////////////////////////////////////
// 									      //
// session.cpp								      //
// 									      //
// This file provides the Session object, representing a Chesley              //
// session. This file provides the protocol independent part of the           //
// implementation.                                                            //
// 									      //
// Matthew Gingell							      //
// gingell@adacore.com							      //
// 									      //
////////////////////////////////////////////////////////////////////////////////

#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <signal.h>
#include <string>
#include <sys/time.h>

#include "chesley.hpp"

using namespace std;

// For now we use a hardcoded timeout in milliseconds.
const int TIME_OUT = 1 * 1000; 

///////////////////////////////
// Static session variables. //
///////////////////////////////

// I/O State.
FILE *Session::in;
FILE *Session::out;
bool  Session::tty;

// Game state
Board   Session::board;
Color   Session::our_color;
bool    Session::op_is_computer;

// Interface mode.
UI_Mode Session::ui_mode;
Protocol Session::protocol;

// Search state.
Search_Engine Session::se;

// Engine state.
bool        Session::running;
uint64      Session::timeout;
const char *Session::prompt;

// Statistics.
int Session::counts_this_game [6][64];
int Session::counts_all_games [6][64];
int Session::num_games;

////////////////////////////////
// Initialize the environment //
////////////////////////////////

void
Session::init_session () {

  // Set initial game state.
  board = Board :: startpos ();
  our_color = BLACK;
  op_is_computer = false;

  // Set initial engine state.
  running = false;
  timeout = 0;

  // Set initial search state.
  se = Search_Engine ();

  // Setup I/O.
  in = stdin;
  out = stdout;
  setvbuf (in, NULL, _IONBF, 0);
  setvbuf (out, NULL, _IONBF, 0);
  tty = isatty (fileno (in));
  
  // Set interface mode.
  ui_mode = tty ? INTERACTIVE : BATCH;
  protocol = NATIVE;

  prompt = ui_mode == INTERACTIVE ? "> " : "";

  // Setup periodic alarm.
  struct itimerval timer;
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 10000;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 10000;
  setitimer(ITIMER_REAL, &timer, NULL);
  signal (SIGALRM, handle_alarm);

  // Initialize statistics.
  memset (counts_this_game, 0, sizeof (counts_this_game));
  memset (counts_all_games, 0, sizeof (counts_all_games));
  num_games = 0;

  return;
}

// Called each time a game begins.
void
Session::collect_new_game () {
  cerr << "collecting new game" << endl;
  num_games++;
}

// Called after each move application.
void 
Session::collect_statistics () {
}

// Called when the a game ends.
void 
Session::collect_game_over () {
  cerr << "collecting game over." << endl;
}

void
Session::handle_alarm (int sig) {
  // We should return from the work loop as quickly as possible if the
  // timeout has elapsed or if there is input waiting from the user.
  if ((timeout > 0 && mclock () > timeout) /* || fdready (fileno (in)) */  )
    se.interrupt_search = true;
}

// Write the command prompt.
void Session::write_prompt ()
{
  if (prompt && (ui_mode == INTERACTIVE))
    fprintf (out, "%s", prompt);
}

/////////////////////////////
// Top level command loop. //
/////////////////////////////

void
Session::cmd_loop ()
{
  bool done = false;

  if (ui_mode == INTERACTIVE)
    fprintf (out, "%s\n", get_prologue ());

  write_prompt ();
  while (true)
    {
      char *line = get_line (in);

      // End session on EOF or false from execute.
      if (!line || !execute (line)) 
	done = true;

      // Discard the malloc'd line return by get_line.
      if (line) free (line);

      // Bail out of the command loop when we're done.
      if (done) break;

      write_prompt ();

      // Hand control over to the 'work' function until input is
      // ready.
      while (!fdready (fileno (in)))
	{
	  work ();
	  usleep (100000);
	}
    }
}

///////////////////
// Flow control. //
///////////////////

// Control is turned over to the engine to do as it wishes until
// either the timeout expires, there is input pending from the user,
// or the interface needs to block and wait for input from the user.
void
Session::work ()
{
  // If we aren't running, return and block for input.
  if (!running) 
    return;

  Status s = get_status ();

  // If the game is over, return and block for input.
  if (s != GAME_IN_PROGRESS) 
    return;

  // If it isn't our turn, return and block for input.
  if (board.to_move () != our_color) 
    return;

  // Otherwise the game is still running and it's our turn, so compute
  // and send a move.
  else
    {
      Move m = find_a_move ();

      // We should never get back a null move.
      assert (!m.is_null());

      // Apply the move.
      bool applied = board.apply (m);
      collect_statistics ();

      // We should never get back a move that doesn't apply.
      assert (applied);

      // Add this move to the triple repetition table.
      se.rt_push (board);

      // Send the move to the client.
      fprintf (out, "move %s\n", board.to_calg (m).c_str ());
    }

  // This move may have ended the game.
  s = get_status ();
  if (s != GAME_IN_PROGRESS)
    handle_end_of_game (s);

  return;
}

// Determine the current status of this game.
Status 
Session :: get_status () {
  Color player = board.to_move ();

  // Check 50 move rule.
  if (board.half_move_clock == 50) 
    return GAME_DRAW;

  // Check for a triple repetition.
  if (se.is_triple_rep (board)) 
    return GAME_DRAW;

  // Check whether there are any moves legal moves for the current
  // player from this position
  if (board.child_count () == 0)
    {
      // This is checkmate. 
      if (board.in_check (player))
	{
	  if (player == WHITE) 
	    return GAME_WIN_BLACK;
	  else
	    return GAME_WIN_WHITE;
	}

      // This is a stalemate.
      else 
	{
	  return GAME_DRAW;
	}
    }
  else
    {
      // Otherwise the game is still in progress.
      return GAME_IN_PROGRESS;
    }
}

// Set session state to reflect that the game has ended.
void
Session::handle_end_of_game (Status s) {

  // Write output to client.
  switch (s)
    {
    case GAME_WIN_WHITE:
      fprintf (out, "1-0 {White mates}\n");
      break;

    case GAME_WIN_BLACK:
      fprintf (out, "0-1 {Black mates}\n");
      break;

    case GAME_DRAW:
      fprintf (out, "1/2-1/2 {Draw}\n");
      break;

    default:
      assert (0);
    }

  // We should halt and block for user input.
  running = false;

  collect_game_over ();

  timeout = 0;
}

// Find a move to play.
Move 
Session::find_a_move () {
  se.interrupt_search = false;
  timeout = mclock () + TIME_OUT;
  return se.choose_move (board, 99);
}

///////////////////////////////////////
// Parse and execute a command line. //
///////////////////////////////////////

bool
Session::execute (char *line) {

  // Pass this command to the protocol specific handler.
  if (protocol == XBOARD && !xbd_execute (line))
    return false;

  // Apply a debugging command.
  if (!debug_execute (line))
    return false;

  // Otherwise, execute a standard command.  
  string_vector tokens = tokenize (line);
  int count = tokens.size ();

  if (count > 0)
    {
      string token = downcase (tokens[0]);

      if (token == "apply" && count > 1)
	{
	  running = false;
	  Move m = board.from_calg (tokens[1]);
	  board.apply (m);
	  return true;
	}

      if (token == "hash")
	{
	  cerr << board.hash << endl;
	  return true;
	}

      if (token == "eval")
	{
	  cerr << eval (board) << endl;
	  return true;
	}

      ////////////////////////
      // Benchmark command. //
      ////////////////////////
      
      if (token == "bench")
	{
	  return bench (tokens);
	}

      ///////////////////
      // Disp command. //
      ///////////////////

      if (token == "disp")
	{
	  fprintf (out, "%s\n", (board.to_ascii ()).c_str ());
	  return true;
	}

      //////////////////
      // Div command. //
      //////////////////

      if (token == "div")
	{
	  if (tokens.size () == 2)
	    {
	      board.divide (to_int (tokens[1]));
	      return true;
	    }
	}

      ////////////////////////////////////////
      // FEN command to output board state. //
      ////////////////////////////////////////

      if (token == "fen")
	{
	  fprintf (out, "%s\n", (board.to_fen ()).c_str ());
	  return true;
	}

      /////////////////////////////////////////
      // EPD command execute an EPD string.  //
      /////////////////////////////////////////

      if (token == "epd")
	{
	  epd (tokens);
	  return true;
	}

      /////////////////////////////////////////////////////////
      // Force command to prevent computer generating moves. //
      /////////////////////////////////////////////////////////

      if (token == "force")
	{
	  running = false;
	  return true;
	}

      ////////////////////
      // Moves command. //
      ////////////////////

      if (token == "moves")
	{
	  Move_Vector moves (board);
	  cerr << moves << endl;
	  return true;
	}

      //////////////////
      // New command. //
      //////////////////

      if (token == "new")
	{
	  collect_new_game ();
	  board = Board :: startpos ();
	  collect_statistics ();
	  se.rt.clear ();
	  our_color = BLACK;
	  running = true;
	}
      //////////////////////
      // Attacks command. //
      //////////////////////

      if (token == "attacks")
	{
	  print_board (board.attack_set (invert_color (board.to_move ())));
	  return true;
	}

      ////////////////////////////////////////
      // Perft position generation command. //
      ////////////////////////////////////////

      if (token == "perft")
	{
	  if (tokens.size () >= 2)
	    {
	      perft (tokens);
	      return true;
	    }
	}

      ////////////////////////////////////
      // The play against self command. //
      ////////////////////////////////////

      if (token == "playself")
	{
	  return play_self (tokens);
	}

      ///////////////////////////
      // setboard FEN command. //
      ///////////////////////////

      if (token == "setboard")
	{
	  board = Board::from_fen (rest (tokens), false);
	  return true;
	}

      ///////////////////
      // Quit command. //
      ///////////////////

      if (token == "quit")
	{
	  return false;
	}

      ///////////////////////
      // Enter xboard ui.  //
      ///////////////////////

      if (token == "xboard")
	{
	  return set_xboard_mode (tokens);
	}

      // Warn about unrecognized commands in interactive mode.
      if (ui_mode == INTERACTIVE)
	{
	  //	  fprintf (out, "Unrecognized command.\n");
	}
    }

  return true;
}

//////////////////
// Status type. //
//////////////////

std::ostream &
operator<< (std::ostream &os, Status s) {
  switch (s)
    {
    case GAME_IN_PROGRESS: os << "GAME_IN_PROGRESS"; break;
    case GAME_WIN_WHITE:   os << "GAME_WIN_WHITE"; break;
    case GAME_WIN_BLACK:   os << "GAME_WIN_BLACK"; break;
    case GAME_DRAW:        os << "GAME_DRAW"; break;
    }
  return os;
}
