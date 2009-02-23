/*
   This file implements various tests and commands for debugging and
   testing Chesley.

   Matthew Gingell
   gingell@adacore.com
*/

#include "chesley.hpp"

#include <string>
#include <iostream>

using namespace std;


// Execute a debugging command.
bool
Session::debug_execute (char *line) {
  return true;
}


// Generate a benchmark. Calculate the best move to depth N.
bool
Session::bench (const string_vector &tokens) {
  int depth = 6;

  if (tokens.size () > 1 && is_number (tokens[1]))
    {
      depth = to_int (tokens[1]);
    }

  timeout = mclock () + 1000.0 * 1000.0 * 1000.0;

  uint64 start = cpu_time();
  Move m = se.choose_move (board, depth);
  uint64 elapsed = cpu_time() - start;
  //  fprintf (out, "Best move at depth %i: %s.\n",
  //	   depth, board.to_calg (m).c_str ());

  cerr << "Best move to depth " << depth << ":" << endl;
  cerr << m << endl;

  fprintf (out, "%.2f seconds elapsed.\n", ((double) elapsed) / 1000.0);
  return true;
}

// Compute possible moves from a position.
bool
Session::perft (const string_vector &tokens)
{
  int depth = 1;

  board = Board::startpos ();

  if (tokens.size () > 1 && is_number (tokens[1]))
    {
      depth = to_int (tokens[1]);
    }

  fprintf (out, "Computing perft to depth %i...\n", depth);

  uint64 start = cpu_time();
  uint64 count = board.perft (depth);
  uint64 elapsed = cpu_time() - start;
  fprintf (out, "moves = %lli\n", count);
  fprintf (out, "%.2f seconds elapsed.\n", ((double) elapsed) / 1000.0);
  return true;
}

// Instruct Chesley to play a game against itself.
bool
Session::play_self (const string_vector &tokens)
{
  board = Board::startpos ();

  while (board.get_status () == GAME_IN_PROGRESS)
    {
      cerr << board << endl << endl;
      Move m = get_move ();
      board.apply (m);
    }

  // Print the position that caused us to break out of the loop.
  cerr << board << endl << endl;

  handle_end_of_game (board.get_status ());

  return true;
}

// Process a string in Extended Position Notation. This can include
// tests, etc.
bool
Session::epd (const string_vector &args)
{
  string_vector tokens = rest (args);

  // The first 4 fields should be a truncated FEN string.
  string fen = join (slice (tokens, 0, 3), " ");
  tokens = slice (tokens, 4);

  // Process EPD opcodes.
  while (1)
    {
      // Exit when we are out of tokens.
      if (tokens.size () == 0)
	{
	  break;
	}

      string opcode = first (tokens);
      tokens = rest (tokens);

      /*********************************************/
      /* Opcode "D<digit> indicating a perft test. */
      /*********************************************/

      if (opcode[0] == 'D')
	{
	  if (opcode.length () != 2 || ! isdigit(opcode [1]))
	    {
	      throw "Bad format in D<digit> opcode";
	    }
	  else
	    {
	      string operand = first (tokens);
	      tokens = rest (tokens);
	      if (!is_number (operand))
		{
		  throw "Bad operand in D<digit> opcode";
		}
	      else
		{
		  int depth = to_int (opcode [1]);
		  uint64 expecting = to_int (operand);

		  /*
		    Format: <PASS|FAIL> <DEPTH> <EXPECTED> <GOT> <ELAPSED> <NPS>
		  */

		  Board b = Board::from_fen (fen, true);
		  uint64 p = b.perft (depth);
		  bool pass = (p == expecting);

		  fprintf (out, "%s %i %llu %llu %.2f\n",
			   pass ? "PASS" : "FAIL",
			   depth, expecting, 0, 0);

		  if (!pass)
		    {
		      fprintf (out, "Position %s fails at depth %i.\n", fen.c_str (), depth);
		    }
		}
	    }
	}
      else
	{
	  // Exit if we encounter and opcode we don't recognize.
	  break;
	}
    }

  return true;
}
