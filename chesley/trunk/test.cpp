/* 
   This file implements various tests and commands for testing
   Chesley.

   Matthew Gingell
   gingell@adacore.com
*/

#include "chesley.hpp"

#include <string>
#include <iostream>

using namespace std; 

// Generate a benchmark. Calculate the best move to depth N.
bool 
Session::bench (const string_vector &tokens) {
  board = Board::startpos ();
  int depth = 6;
  
  if (tokens.size () > 1 && is_number (tokens[1]))
    {
      depth = to_int (tokens[1]);
    }

  double start_time = user_time (); 
  se.score_count = 0;
  se.max_depth = depth;
  se.choose_move (board);
  double elapsed = user_time () - start_time;
  
  fprintf (out, "Best move at depth %i.\n", depth); 
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

  board = Board::startpos ();

  if (tokens.size () > 1 && is_number (tokens[1]))
    {
      depth = to_int (tokens[1]);
    }

  fprintf (out, "Computing perft to depth %i...\n", depth);

  double start = user_time (); 
  uint64 count = board.perft (depth);
  double elapsed = user_time () - start;
  fprintf (out, "moves = %lli\n", count); 
  fprintf (out, "%.2f seconds elapsed.\n", elapsed);
  fprintf (out, "%.2f moves/second.\n", ((double) count) / elapsed);  

  return true;
}

// Instruct Chesley to play a game against itself.
bool 
Session::play_self (const string_vector &tokens) 
{
  board = Board::startpos ();

  try 
    {
      while (1)
	{
	  cerr << board << endl;
	  Move m = se.choose_move (board);
	  board.apply (m);
	}
    }
  catch (Game_Over s)
    {
      handle_end_of_game (s.status);
    }
  
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
		  double start = user_time ();
		  uint64 p = b.perft (depth);
		  double elapsed = user_time () - start;
		  bool pass = (p == expecting);

		  fprintf (out, "%s %i %llu %llu %.2f\n",
			   pass ? "PASS" : "FAIL", 
			   depth, expecting, p, elapsed);

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
