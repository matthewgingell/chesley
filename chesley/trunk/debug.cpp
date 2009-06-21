////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// debug.cpp                                                                  //
//                                                                            //
// This file implements various tests and commands for debugging and          //
// testing Chesley.                                                           //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <fstream>
#include <string>

#include "chesley.hpp"

using namespace std;

///////////////////////////////////////////////////////////////
// Generate a benchmark. Calculate the best move to depth N. //
///////////////////////////////////////////////////////////////

bool
Session::bench (const string_vector &tokens) {
  int depth = 6;
  if (tokens.size () > 1 && is_number (tokens[1]))
    depth = to_int (tokens[1]);
  running = false;
  Move m = se.choose_move (board, depth);
  return true;
}

/////////////////////////////////////////////
// Compute possible moves from a position. //
/////////////////////////////////////////////

bool
Session::perft (const string_vector &tokens)
{
  int depth = 1;

  if (tokens.size () > 1 && is_number (tokens[1]))
    depth = to_int (tokens[1]);

  fprintf (out, "Computing perft to depth %i...\n", depth);
  uint64 start = cpu_time();
  uint64 count = board.perft (depth);
  uint64 elapsed = cpu_time() - start;
  fprintf (out, "moves = %llu\n", count);
  fprintf (out, "%.2f seconds elapsed.\n", ((double) elapsed) / 1000.0);
  return true;
}

/////////////////////////////////////////////////////
// Instruct Chesley to play a game against itself. //
/////////////////////////////////////////////////////

bool
Session::play_self (const string_vector &tokens IS_UNUSED)
{
  Status s;
  board = Board::startpos ();
  running = true;
  while ((s = get_status ()) == GAME_IN_PROGRESS)
    {
      cout << board << endl << endl;
      Move m = find_a_move ();
      board.apply (m);
    }
  cout << board << endl << endl;
  handle_end_of_game (s);
  running = false;

  return true;
}

/////////////////////////////////////////////////////////////////////
// Dump a vector representing the pawn structure of this position. //
/////////////////////////////////////////////////////////////////////

bool
Session::dump_pawns (const string_vector &tokens IS_UNUSED)
{
  ofstream out;
  out.open ("pawn_struct");

  while (1)
    {
      Status s;
      board = Board::startpos ();
      while ((s = get_status ()) == GAME_IN_PROGRESS)
        {
          // dump white pawns vector.
          bitboard white_pawns = board.pawns & board.white;
          for (int i = 0; i < 64; i++)
            {
              if (test_bit (white_pawns, i))
                out << "1 ";
              else
                out << "0 ";
            }

          // dump black pawns vector.
          bitboard black_pawns = board.pawns & board.black;
          for (int i = 0; i < 64; i++)
            {
              if (test_bit (black_pawns, i))
                out << "1 ";
              else
                out << "0 ";
            }
          out << endl;

          cout << board << endl << endl;
          Move m = find_a_move ();
          board.apply (m);
        }
      cout << board << endl << endl;
      handle_end_of_game (s);
    }
  return true;
}

////////////////////////////////////////////////////////////////
// Check that hash keys are correctly generated to depth 'd'. //
////////////////////////////////////////////////////////////////

int
test_hashing_rec (const Board &b, int depth) {
  Move_Vector moves (b);
  int pass = 0;

  if (b.hash == b.gen_hash ())
    pass++;
  else
    cout << "FAIL at depth: " << depth << endl;

  if (depth == 0) return 1;

  for (int i = 0; i < moves.count; i++)
    {
      Board c = b;
      if (c.apply (moves[i])) pass += test_hashing_rec (c, depth - 1);
    }

  return pass;
}

void
Session::test_hashing (int d) {
  int pass = test_hashing_rec (board, d);
  cout << pass << endl;
}

//////////////////////////////////////////////////////////////////////
// Process a string in Extended Position Notation. This can include //
// tests, etc.                                                      //
//////////////////////////////////////////////////////////////////////

bool
Session::epd (const string_vector &args)
{
  string_vector tokens = rest (args);

  // The first 4 fields should be a truncated FEN string.
  string fen = join (slice (tokens, 0, 3), " ");
  tokens = slice (tokens, 4);
  Board b = Board::from_fen (fen, true);

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

      ///////////////////////////////////////////////
      // Opcode "D<digit> indicating a perft test. //
      ///////////////////////////////////////////////

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
                  ///////////////////////////////////////
                  // Format: <PASS|FAIL> <DEPTH> <GOT> //
                  ///////////////////////////////////////

                  int depth = to_int (opcode [1]);
                  uint64 expecting = to_int (operand);
                  uint64 p = b.perft (depth);
                  bool pass = (p == expecting);
                  fprintf (out, "%s %llu\n", pass ? "PASS" : "FAIL", p);

                  if (!pass)
                    fprintf  (out,
                              "Position %s fails at depth %i.\n",
                              fen.c_str (), depth);
                }
            }
        }

      else if (opcode == "bm")
        {
          Move best = b.from_san (first (tokens));
          cout << "Trying " << fen << " bm " << b.to_san (best) << endl;
          se.set_fixed_time (15 * 1000);
          se.post = true;
          running = true;
          Move m = se.choose_move (b, 100);
          running = false;
          (m == best) ? cout << "PASS: " : cout << "FAIL: ";
          cout << fen << " bm " << b.to_san (best) << endl << endl;;
          tokens == rest (tokens);
        }

      else
        {
          // Exit if we encounter and opcode we don't recognize.
          break;
        }
    }

  return true;
}
