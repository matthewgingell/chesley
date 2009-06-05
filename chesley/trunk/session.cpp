////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// session.cpp                                                                //
//                                                                            //
// This file provides the Session object, representing a Chesley              //
// session. This file provides the protocol independent part of the           //
// implementation.                                                            //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
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
const char *Session::prompt;

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

  // Set initial search state.
  se = Search_Engine ();
  se.post = true;

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

  // Setup periodic 100 Hz alarm.
  struct itimerval timer;
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 100 * 1000;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 100 * 1000;
  setitimer(ITIMER_REAL, &timer, NULL);
  signal (SIGALRM, handle_alarm);
}

void
Session::handle_alarm (int sig IS_UNUSED) {
  se.poll (mclock ());
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
    {
      return;
    }

  // Otherwise the game is still running and it's our turn, so compute
  // and send a move.
  else
    {
      Move m = find_a_move ();

      // We should never get back a null move from the search engine.
      assert (!m.is_null());

      // Apply the move.
      bool applied = board.apply (m);

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

#if 0
  // Check for loss on time.
  if (se.controls.time_remaining == 0) 
    {
      if (player == WHITE)
        return GAME_WIN_BLACK;
      else
        return GAME_WIN_WHITE;
    }
#endif

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
}

// Find a move to play.
Move
Session::find_a_move () {
  uint64 start_time = mclock ();
  Move m = se.choose_move (board, 99);
  uint64 elapsed = mclock () - start_time;

  // The caller is responsible for managing the time and move limits
  // set in the search engine, since the search engine can't know what
  // is being done with the results it returns.
  if (((uint64) se.controls.time_remaining) > elapsed)
    {
      se.controls.time_remaining -= elapsed;
    }
  else
    {
      se.controls.time_remaining = 0;
    }

  if (se.controls.moves_remaining > 0)
    {
      se.controls.moves_remaining--;
    }

  return m;
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

      /////////////////////////
      // Debugging commands. //
      /////////////////////////

      // Apply a move to the current position.
      if (token == "apply" && count > 1)
        {
          running = false;
          Move m = board.from_calg (tokens[1]);
          board.apply (m);
        }

      // Write a bitboard of attacks against the side to move.
      else if (token == "attacks")
        {
          print_board (board.attack_set (invert (board.to_move ())));
        }

      // Search the current position to a fixed depth.
      else if (token == "bench")
        {
          bench (tokens);
        }

      // Output the perft score for each child.
      else if (token == "div")
        {
          if (tokens.size () == 2)
            {
              board.divide (to_int (tokens[1]));
            }
        }

      // Dump pawn structure to a file.
      else if (token == "dumppawns")
        {
          dump_pawns (tokens);
        }

      // Execute an epd string.
      else if (token == "epd")
        {
          epd (tokens);
        }

      // Output the static evaluation for this position.
      else if (token == "eval")
        {
          cerr << Eval (board).score () << endl;
        }

      // Output the hash key for this position.
      else if (token == "hash")
        {
          cerr << board.hash << endl;
        }

      // Output the legal moves from this position.
      else if (token == "moves")
        {
          Move_Vector moves (board);
          cerr << moves << endl;
        }

      // Output the number of positions exactly N moves from this
      // position.
      else if (token == "perft")
        {
          if (tokens.size () >= 2)
            {
              perft (tokens);
            }
        }

      // Play an engine vs. engine game on the console.
      else if (token == "playself")
        {
          play_self (tokens);
        }

      ///////////////////////
      // Display commands. //
      ///////////////////////

      // Write an ASCII are board.
      else if (token == "disp")
        {
          fprintf (out, "%s\n", (board.to_ascii ()).c_str ());
        }

      // Display the current time controls.
      else if (token == "dtc")
        {
          fprintf (out, "mode:            ");
          switch (se.controls.mode)
            {
            case CONVENTIONAL: fprintf (out, "CONVENTIONAL"); break;
            case ICS:          fprintf (out, "ICS"); break;
            case EXACT:        fprintf (out, "EXACT"); break;
            }
          fprintf (out, "\n");

          fprintf (out, "moves_ptc:       %i\n", se.controls.moves_ptc);
          fprintf (out, "time_ptc:        %i\n", se.controls.time_ptc);
          fprintf (out, "increment:       %i\n", se.controls.increment);
          fprintf (out, "fixed_time:      %i\n", se.controls.fixed_time);
          fprintf (out, "fixed_depth:     %i\n", se.controls.fixed_depth);
          fprintf (out, "time_remaining:  %i\n", se.controls.time_remaining);
          fprintf (out, "moves_remaining: %i\n", se.controls.moves_remaining);
        }

      // Write the current position as a fen string.
      else if (token == "fen")
        {
          fprintf (out, "%s\n", (board.to_fen ()).c_str ());
        }

      ///////////////////////////////////////////////
      // Commands for play a game with the engine. //
      ///////////////////////////////////////////////

      // Set user to play black.
      else if (token == "black")
        {
          board.set_color (BLACK);
          our_color = WHITE;
        }

      // Put the engine into force mode.
      else if (token == "force")
        {
          running = false;
        }

      // Leave force mode.
      else if (token == "go") {
        our_color = board.to_move ();
        running = true;
      }

      // Play a move.
      else if ((token == "move" || token == "usermove") && count > 1)
        {
          Move m = board.from_calg (tokens[1]);
          bool applied = board.apply (m);

          // The client should never pass us a move that doesn't
          // apply.
          assert (applied);

          // This move may have ended the game.
          Status s = get_status ();
          if (s != GAME_IN_PROGRESS)
            {
              handle_end_of_game (s);
            }
        }

      // Start a new game.
      else if (token == "new")
        {
          board = Board :: startpos ();
          se.reset ();
          our_color = BLACK;
          running = true;
        }

      // Swap colors between the engine and the user.
      else if (token == "playother")
        {
          our_color = invert (our_color);
        }

      // Set the board from a fen string.
      else if (token == "setboard")
        {
          board = Board::from_fen (rest (tokens), false);
        }

      // Set user to play white.
      else if (token == "white")
        {
          board.set_color (WHITE);
          our_color = BLACK;
        }

      ////////////////////////////////////
      // Time control related commands. //
      ////////////////////////////////////

      // level MPC BASE INC command.
      else if (token == "level")
        {
          tokens = rest (tokens);

          // Check we've been passed enough arguments.
          if (tokens.size () == 3)
            {
              int moves_per_control = 0, time_per_control = 0, increment = 0;
              string field = first (tokens);

              // Field 1: Parse the moves per time control.
              if (!is_number (field)) assert (0);
              moves_per_control = to_int (field);
              tokens = rest (tokens);

              // Field 2: Parse the number of seconds per time control. This
              // may be written as "<minutes>:<second>".
              field = first (tokens);
              size_t idx = field.find (':');
              int minutes = 0, seconds = 0;
              if (idx != string::npos)
                {
                  if (idx != 0) minutes = to_int (field.substr (0, idx));
                  seconds = to_int (field.substr (idx + 1));
                }
              else
                {
                  minutes = to_int (field);
                }
              time_per_control = 1000 * (60 * minutes + seconds);
              tokens = rest (tokens);

              // Field 3: Parse to incremental time bonus.
              field = first (tokens);
              increment = to_int (field);

              // Set search engine time controls.
              se.set_level (moves_per_control, time_per_control, increment);
            }
          else
            {
              // TODO: Raise a UI exception.
            }
        }

      // Set fixed depth move mode.
      else if (token == "sd")
        {
          if (count > 1)
            {
              se.set_fixed_depth (to_int (tokens[1]));
            }
        }

      // Set fixed time move mode.
      else if (token == "st")
        {
          if (count > 1)
            {
              se.set_fixed_time (1000 * to_int (tokens[1]));
              
            }
        }

      // Set the clock.
      else if (token == "time")
        {
          if (count > 1)
            {
              se.set_time_remaining (10 * to_int (tokens[1]));
#if 0
              fprintf (out, "mode:            ");
              switch (se.controls.mode)
                {
                case CONVENTIONAL: fprintf (out, "CONVENTIONAL"); break;
                case ICS:          fprintf (out, "ICS"); break;
                case EXACT:        fprintf (out, "EXACT"); break;
                }
              fprintf (out, "\n");
              fprintf (out, "moves_ptc:       %i\n", se.controls.moves_ptc);
              fprintf (out, "time_ptc:        %i\n", se.controls.time_ptc);
              fprintf (out, "increment:       %i\n", se.controls.increment);
              fprintf (out, "fixed_time:      %i\n", se.controls.fixed_time);
              fprintf (out, "fixed_depth:     %i\n", se.controls.fixed_depth);
              fprintf (out, "time_remaining:  %i\n", se.controls.time_remaining);
              fprintf (out, "moves_remaining: %i\n", se.controls.moves_remaining);
#endif
            }
        }

      // otime N command
      else if (token == "otime")
        {
          // For now we don't care about the amount of time remaining
          // to the opponent.
        }

      //////////////////////
      // Session commands //
      //////////////////////

      else if (token == "quit" || token == "exit")
        {
          return false;
        }

      else if (token == "xboard")
        {
          return set_xboard_mode (tokens);
        }

      ///////////////////////
      // Enter xboard ui.  //
      ///////////////////////

      // Warn about unrecognized commands in interactive mode.
      else if (ui_mode == INTERACTIVE)
        {
          fprintf (out, "Unrecognized command.\n");
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
