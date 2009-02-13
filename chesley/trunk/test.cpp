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
  
  fprintf (out, "At depth %i.\n", depth); 
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
