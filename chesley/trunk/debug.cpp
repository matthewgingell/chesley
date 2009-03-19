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


/**************************************/
/* Parse and execute a debug command. */
/**************************************/
bool
Session::debug_execute (char *line) {
  string_vector tokens = tokenize (line);
  int count = tokens.size ();

  // Execute a standard command.
  if (count > 0)
    {
      string token = downcase (tokens[0]);

      if (token == "test_hashing")
	{
	  // For all positions to depth N, compute the hash
	  // incrementally and from scratch and make sure each method
	  // generates the same value.

	  int depth = 5;

	  if (tokens.size () > 1 && is_number (tokens[1]))
	    {
	      depth = to_int (tokens[1]);
	    }

	  test_hashing (depth);
	}

      if (token == "ab")
	{
	  // Call alpha on this position with the specified window and
	  // depth.

	  timeout = mclock () + 1000.0 * 1000.0 * 1000.0;
	  se.interrupt_search = false;

	  if (tokens.size () == 4)
	    {
	      int depth = to_int (tokens[1]);
	      int alpha = to_int (tokens[2]);
	      int beta =  to_int (tokens[3]);
	      
	      cerr << "Calling alphabeta with ("
		   << "depth:" << depth << " "
		   << "alpha:" << alpha << " "
		   << "beta:" << beta 
		   << ")" << endl;

	      se.calls_to_alpha_beta = 0;
	      //	      cerr << se.alpha_beta (board, depth, alpha, beta) << endl;
	      fprintf (out, "%lli calls_to_alpha_beta.\n", se.calls_to_alpha_beta);
	    }
	}
    }

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

  //  Move m = se.alpha_beta (board, depth, -INFINITY, +INFINITY);

  uint64 elapsed = cpu_time() - start;
  //  fprintf (out, "Best move at depth %i: %s.\n",
  //	   depth, board.to_calg (m).c_str ());

  cerr << "Best move to depth " << depth << ":" << endl;
  cerr << m << endl;

  fprintf (out, "%.2f seconds elapsed.\n", ((double) elapsed) / 1000.0);

  fprintf (out, "%lli calls_to_alpha_beta.\n", se.calls_to_alpha_beta);

  return true;
}

// Compute possible moves from a position.
bool
Session::perft (const string_vector &tokens)
{
  int depth = 1;

  //  board = Board::startpos ();

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

  //  while (status == GAME_IN_PROGRESS)
    {
      cerr << board << endl << endl;
      // Move m = get_move ();
      // board.apply (m);
    }

  // Print the position that caused us to break out of the loop.
  cerr << board << endl << endl;

  //  handle_end_of_game (status);

  return true;
}


// Check that hash keys are correctly generated to depth 'd'.
int
test_hashing_rec (const Board &b, int depth) {
  Board_Vector children (b);
  int pass = 0;

  if (b.hash == b.gen_hash ())
    {
      pass++;
    }
  else
    {
      cerr << "FAIL at depth: " << depth << endl;
      //      cerr << b << endl;
    }

  if (depth == 0)
    {
      return 1;
    }

  for (int i = 0; i < children.count; i++)
    {
      pass += test_hashing_rec (children [i], depth - 1);
    }

  return pass;
}

void
Session::test_hashing (int d) {
  int pass = test_hashing_rec (board, d);
  cerr << pass << endl;
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

		  fprintf (out, "%s %i %llu %i %i\n",
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
