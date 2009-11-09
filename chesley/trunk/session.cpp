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

#include <cstdio>
#include <cstring>
#include <signal.h>
#include <string>

#include "chesley.hpp"

using namespace std;

//////////////////////////////
// Static session variables //
//////////////////////////////

// Hack
bool xboard = false;

// I/O State.
FILE *Session::in;
FILE *Session::out;
bool  Session::tty;

// Game state
Board       Session::board;
Move_Vector Session::pv;
Color       Session::our_color;
bool        Session::op_is_computer;

// Interface mode.
UI_Mode  Session::ui_mode;
Protocol Session::protocol;

// Search engine.
Search_Engine Session::se;

// Engine state.
bool        Session::running;
bool        Session::ponder_enabled = false;
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
  pv.clear ();
  
  // Set initial engine state.
  running = false;

  // Set initial search state.
  se = Search_Engine ();
  se.post = true;

  // Setup I/O.
  in = stdin;
  out = stdout;

#ifndef _WIN32
  tty = isatty (fileno (in));
#else
  tty = true;
#endif

  // Set interface mode.
  ui_mode = tty ? INTERACTIVE : BATCH;
  protocol = NATIVE;

  prompt = ui_mode == INTERACTIVE ? "> " : "";
}

// Write the command prompt.
void Session::write_prompt ()
{
  if (prompt && (ui_mode == INTERACTIVE))
    fprintf (out, "%s", prompt);
}

////////////////////////////
// Top level command loop //
////////////////////////////

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

//////////////////
// Flow control //
//////////////////

// Control is turned over to the engine to do as it wishes until
// either the timeout expires, there is input pending from the user,
// or the interface needs to block and wait for input from the user.
void
Session::work ()
{
  // If we aren't running, return and block for input.
  if (!running) return;

  // If the game is over, return and block for input.
  if (get_status (board) != GAME_IN_PROGRESS) return;

  // If it isn't our turn, possibly ponder then return.
  if (board.to_move () != our_color) 
    {
      if (ponder_enabled && pv.count >= 2)
        {
          Board to_ponder = board;

          if (!to_ponder.apply (pv[1]))
            {
              assert (0);
            }
          
          if (get_status (to_ponder) == GAME_IN_PROGRESS)
            {
              se.do_ponder (to_ponder);
            }
        }

      return;
    }

  // Otherwise the game is still running and it's our turn, so compute
  // and send a move.
  Move m = find_a_move ();

  // We should never get back a null move from the search engine.
  assert (m != NULL_MOVE);

  // Apply the move.
  bool applied = board.apply (m);

  // We should never get back a move that doesn't apply.
  assert (applied);

  // Add the resulting position to the repetition table.
  se.rt_push (board);

  // Send the move to the client.
  fprintf (out, "move %s\n", board.to_calg (m).c_str ());

  // This move may have ended the game.
  Status s = get_status (board);
  if (s != GAME_IN_PROGRESS)
    handle_end_of_game (s);
}

// Determine the current status of this game.
Status
Session :: get_status (Board &b) {
  Color player = b.to_move ();

  // Check 50 move rule.
  if (b.half_move_clock == 100)
    return GAME_DRAW;

  // Check for a triple repetition.
  if (se.is_triple_rep (b))
    return GAME_DRAW;

  // Check whether there are any moves legal moves for the current
  // player from this position
  if (b.child_count () == 0)
    {
      // This is checkmate.
      if (b.in_check (player))
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
  pv.clear ();
  se.compute_pv (board, 256, pv);
  uint64 elapsed = mclock () - start_time;

  // The caller is responsible for managing the time and move limits
  // set in the search engine, since the search engine can't know what
  // is being done with the results it returns.

  if (((uint64) se.controls.time_remaining) > elapsed)
    {
      se.controls.time_remaining -= (int32) elapsed;
    }
  else
    {
      se.controls.time_remaining = 0;
    }

  if (se.controls.moves_remaining > 0)
    {
      se.controls.moves_remaining--;
    }
  else
    {
      se.controls.moves_remaining = se.controls.moves_ptc;
    }

  return pv[0];
}

// This function must be called periodically to implement timeouts.
void
Session::poll () {
  int64 now = mclock ();

  // We have reached the search deadline or there is input pending on STDIN.
  if ((se.controls.deadline > 0 && now >= se.controls.deadline) ||
      (fdready (fileno (in))))
    {
      se.controls.interrupt_search = true;
      throw SEARCH_INTERRUPTED;
    }
}

/////////////////
// Status type //
/////////////////

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
