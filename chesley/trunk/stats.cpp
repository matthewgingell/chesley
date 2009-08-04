////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// stats.hpp                                                                  //
//                                                                            //
// Code to analyze chess games and collect statistics.                        //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <iomanip>
#include <iostream>
#include <Cmath>

#include "chesley.hpp"

using namespace std;

int flip_left_right[64] =
{
   7,  6,  5,  4,  3,  2,  1,  0,
  15, 14, 13, 12, 11, 10,  9,  8,
  23, 22, 21, 20, 19, 18, 17, 16,
  31, 30, 29, 28, 27, 26, 25, 24,
  39, 38, 37, 36, 35, 34, 33, 32,
  47, 46, 45, 44, 43, 42, 41, 40,
  55, 54, 53, 52, 51, 50, 49, 48,
  63, 62, 61, 60, 59, 58, 57, 56
};

int flip_white_black[64] =
{
   0,   1,   2,   3,   4,   5,   6,   7,
   8,   9,  10,  11,  12,  13,  14,  15,
  16,  17,  18,  19,  20,  21,  22,  23,
  24,  25,  26,  27,  28,  29,  30,  31,
  32,  33,  34,  35,  36,  37,  38,  39,
  40,  41,  42,  43,  44,  45,  46,  47,
  48,  49,  50,  51,  52,  53,  54,  55,
  56,  57,  58,  59,  60,  61,  62,  63
};

// Output four columns: The first is Chesley's eval, followed by win
// count, loss count, and draw count.
void collect_eval_vs_winp  (const char *filename) {
  const int min_score = -1000;
  const int max_score = +1000;
  const int range = max_score - min_score + 1;

  int count = 0;
  int wins[range];
  int losses[range];
  int draws[range];
  memset (wins, 0, sizeof (wins));
  memset (losses, 0, sizeof (losses));
  memset (draws, 0, sizeof (draws));
  PGN pgn;
  pgn.open (filename);

  while (true)
    {
      count++;

#if 0
      if (count == 100)
        break;
#endif

      if (count % 1000 == 0)
        {
          cerr << "reading #" << count << "..." << endl;
        }

      Game g = pgn.read_game ();
      Score last_score = 0;
      Score stable_count = 0;
      if (pgn.status != PGN::END_OF_FILE)
        {
          Board b = Board::startpos ();
          for (uint32 i = 0; i < g.moves.size (); i++)
            {
              b.apply (g.moves[i]);
              int score;

              score = (b.material[WHITE] - b.material[BLACK]) / 1;
              if (score == last_score)
                {
                  stable_count += 1;
                }
              else
                {
                  stable_count = 0;
                }

              last_score = score;

              if (stable_count <= 5)
                {
                  continue;
                }

              score = min (score, max_score);
              score = max (score, min_score);

              int index = score - min_score;

              if (g.winner == WHITE)
                {
                  wins[index]++;
                }
              else if (g.winner == BLACK)
                {
                  losses[index]++;
                }
              else
                {
                  draws[index]++;
                }
            }
        }
      else
        {
          break;
        }
    }

  for (int i = -1000; i <= 1000; i++)
    {
      int idx = i - min_score;
      double count = wins[idx] + losses[idx] + losses[idx];
      double winp = (double) wins[idx] / count;

      if (count > 10)
        {
          cout << setw (15) << i;
          cout << setw (15) << count;
          cout << setw (15) << winp;
          cout << endl;
        }
    }

  cerr << "done." << endl;
  pgn.close ();
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//  void gen_psq_tables  (const char *filename);                             //
//                                                                           //
//  Generate piece square tables from a .pgn file. We try the same           //
//  approach taken by Bayesian spam filters, but instead of computing        //
//  (for instance) the probability an email is spam given that it            //
//  contain the work Viagra, we compute the probability a position is        //
//  a win given that there's a knight on C3.                                 //
//                                                                           //
// The model is as follows:                                                  //
//                                                                           //
// 1.) We compute the prior probability p(w) that a position is a win        //
//     for white.                                                            //
//                                                                           //
// 2.) We compute the prior probability p(f) that a position matches a       //
//     feature.                                                              //
//                                                                           //
// 3.) We compute the conditional probability p(f|w) that, given a           //
//     position is a win for white, the position matches a feature.          //
//                                                                           //
// 4.) We apply Bayes law to compute the probability p(w|f) that,            //
//     given a position matches a feature, the position is a win.            //
//                                                                           //
// 5.) We convert the win probability p(w|f) to an equivalent material       //
//     value in centipawns by assuming the two are related by a              //
//     logistic function.                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

void gen_psq_tables (const char *filename) {
  uint32 games_count = 0;
  double won_positions_count = 0;
  double drawn_positions_count = 0;
  double lost_positions_count = 0;
  double positions_count = 0;
  double f_knight_count [64];
  double f_knight_given_win_count [64];
  double f_knight_given_loss_count [64];
  double f_knight_given_draw_count [64];

  memset (f_knight_count, 0, sizeof (f_knight_count));
  memset (f_knight_given_win_count, 0, sizeof (f_knight_count));
  memset (f_knight_given_loss_count, 0, sizeof (f_knight_count));
  memset (f_knight_given_draw_count, 0, sizeof (f_knight_count));

  PGN pgn;
  pgn.open (filename);
  while (true)
    {
      // Count the number of games in the stream.
      games_count++;

      // Output progress.
      if (games_count % 1000 == 0)
        cerr << "reading #" << games_count << "..." << endl;

      // Read a game.
      Game g = pgn.read_game ();
      if (pgn.status == PGN::END_OF_FILE) break;

      // Play out a game.
      Board b = Board::startpos ();
      for (uint32 i = 0; i < g.moves.size (); i++)
        {
          // We are only interested in positions with a small material
          // difference.
          // if (abs (b.material[WHITE] - b.material[BLACK]) > 2 * PAWN_VAL)
          // continue;

          // Iterate over features and insert this position and it's
          // left-right mirror image for white.
          positions_count += 2;
          if (g.winner == WHITE) won_positions_count += 2;
          if (g.winner == BLACK) lost_positions_count += 2;
          if (g.winner == NULL_COLOR) drawn_positions_count += 2;
          
          for (int idx = 0; idx < 64; idx++)
            {
              // Insert the position for white.
              if (test_bit (b.get_knights (WHITE), idx))
                {
                  f_knight_count[idx]++;
                  f_knight_count[flip_left_right[idx]]++;
                  if (g.winner == WHITE)
                    {
                      f_knight_given_win_count[idx]++;
                      f_knight_given_win_count[flip_left_right[idx]]++;
                    }
                  else if (g.winner == BLACK)
                    {
                      f_knight_given_loss_count[idx]++;
                      f_knight_given_loss_count[flip_left_right[idx]]++;
                    }
                  else if (g.winner == NULL_COLOR)
                    {
                      f_knight_given_draw_count[idx]++;
                      f_knight_given_draw_count[flip_left_right[idx]]++;
                    }
                }
            }

          // Iterate over features and insert this position and it's
          // left-right mirror image for black.
          positions_count += 2;
          if (g.winner == BLACK) won_positions_count += 2;
          if (g.winner == WHITE) lost_positions_count += 2;
          if (g.winner == NULL_COLOR) drawn_positions_count += 2;

          for (int idx = 0; idx < 64; idx++)
            {
              int bidx = flip_white_black [idx];
              if (test_bit (b.get_knights (BLACK), bidx))
                {
                  f_knight_count[bidx]++;
                  f_knight_count[flip_left_right[bidx]]++;
                  if (g.winner == BLACK)
                    {
                      f_knight_given_win_count[bidx]++;
                      f_knight_given_win_count[flip_left_right[bidx]]++;
                    }
                  else if (g.winner == WHITE)
                    {
                      f_knight_given_loss_count[bidx]++;
                      f_knight_given_loss_count[flip_left_right[bidx]]++;
                    }
                  else if (g.winner == NULL_COLOR)
                    {
                      f_knight_given_draw_count[bidx]++;
                      f_knight_given_draw_count[flip_left_right[bidx]]++;
                    }
                }
            }
          b.apply (g.moves[i]);
        }
    }

  // Output statistics
  cerr << "positions_count: " << positions_count << endl;
  cerr << endl;
  cerr << "won_positions_count: " << won_positions_count << endl;
  cerr << "p(w) = ";
  double pw = won_positions_count / positions_count;
  cerr << pw;
  cerr << endl;
  cerr << endl;
  cerr << "lost_positions_count: " << lost_positions_count << endl;
  cerr << "p(l) = ";
  double pl = lost_positions_count / positions_count;
  cerr << pl;
  cerr << endl;
  cerr << endl;
  cerr << "drawn_positions_count: " << drawn_positions_count << endl;
  cerr << "p(d) = ";
  double pd = drawn_positions_count / positions_count;
  cerr << pd;
  cerr << endl;
  cerr << endl;

  double pf[64];
  cerr << "p(f):" << endl;
  for (int row = 7; row >= 0; row--)
    {
      for (int file = 0; file < 8; file++)
        {
          int idx = Board::to_idx (row, file);
          if (positions_count > 0)
            {
              pf[idx] = f_knight_count[idx] / positions_count;
            }
          else
            {
              pf[idx] = 0;
            }
          cerr << setiosflags (ios::fixed);
          cerr << setprecision (4);
          cerr << pf[idx];
          cerr << ", ";
        }
      cerr << endl;
    }

  cerr << endl;

  double pfw[64];
  cerr << "p(f|w):" << endl;
  for (int row = 7; row >= 0; row--)
    {
      for (int file = 0; file < 8; file++)
        {
          int idx = Board::to_idx (row, file);
          if (won_positions_count > 0)
            {
              pfw[idx] = f_knight_given_win_count[idx] / won_positions_count;
            }
          else
            {
              pfw[idx] = 0;
            }
          cerr << setiosflags (ios::fixed);
          cerr << setprecision (4);
          cerr << pfw[idx];
          cerr << ", ";
        }
      cerr << endl;
    }

  cerr << endl;
  
  double pfd[64];
  cerr << "p(f|d):" << endl;
  for (int row = 7; row >= 0; row--)
    {
      for (int file = 0; file < 8; file++)
        {
          int idx = Board::to_idx (row, file);
          if (drawn_positions_count > 0)
            {
              pfd[idx] = 
                f_knight_given_draw_count[idx] / drawn_positions_count;
            }
          else
            {
              pfd[idx] = 0;
            }
          cerr << setiosflags (ios::fixed);
          cerr << setprecision (4);
          cerr << pfd[idx];
          cerr << ", ";
        }
      cerr << endl;
    }

  cerr << endl;

  //
  // By Bayes Law:
  //
  // p (w|f) = p(f|w) * p(w)
  //           -------------
  //                p(f)

  double pwf[64];
  cerr << "p(w|f):" << endl;
  for (int row = 7; row >= 0; row--)
    {
      for (int file = 0; file < 8; file++)
        {
          int idx = Board::to_idx (row, file);
          if (pf[idx] == 0)
            {
              pwf[idx] = 0;
            }
          else
            {
              pwf[idx] = pfw[idx] * pw / pf[idx];
            }
          cerr << setiosflags (ios::fixed);
          cerr << setprecision (2);
          cerr << pwf[idx];
          cerr << ", ";
        }
      cerr << endl;
    }

  cerr << endl;

  double pdf[64];
  cerr << "p(d|f):" << endl;
  for (int row = 7; row >= 0; row--)
    {
      for (int file = 0; file < 8; file++)
        {
          int idx = Board::to_idx (row, file);
          if (pf[idx] == 0)
            {
              pdf[idx] = 0;
            }
          else
            {
              pdf[idx] = pfd[idx] * pd / pf[idx];
            }
          cerr << setiosflags (ios::fixed);
          cerr << setprecision (2);
          cerr << pdf[idx];
          cerr << ", ";
        }
      cerr << endl;
    }

  cerr << endl;

  double pwdf[64];
  cerr << "p (win or draw | f):" << endl;
  for (int row = 7; row >= 0; row--)
    {
      for (int file = 0; file < 8; file++)
        {
          int idx = Board::to_idx (row, file);
          pwdf[idx] = pwf[idx] + pdf[idx];
          cerr << setiosflags (ios::fixed);
          cerr << setprecision (2);
          cerr << pwdf[idx];
          cerr << ", ";
        }
      cerr << endl;
    }

  cerr << endl;
  
  double cpawns[64];
  cerr << "p (win or draw | f) in centipawns:" << endl;
  for (int row = 7; row >= 0; row--)
    {
      for (int file = 0; file < 8; file++)
        {
          int idx = Board::to_idx (row, file);
          cpawns[idx] = round (400 * log10 (pwdf[idx] / (1 - pwdf[idx])));
          cerr << setiosflags (ios::fixed);
          cerr << setprecision (0);
          cerr << setw (5);
          cerr << cpawns[idx];
          cerr << ", ";
        }
      cerr << endl;
    }

  cerr << endl;

  cerr << "Weighted average of wins and draws in centipawns:" << endl;
  cerr << setiosflags (ios::fixed);
  cerr << setprecision (0);
  for (int row = 7; row >= 0; row--)
    {
      for (int file = 0; file < 8; file++)
        {
          int idx = Board::to_idx (row, file);

          // Compute the probability p(w|f) excluding draws.
          double won_or_lost_count = 
            won_positions_count + lost_positions_count;
          double pfw = f_knight_given_win_count[idx] / won_positions_count;
          double pw = won_positions_count / won_or_lost_count;
          double pf = (f_knight_given_win_count[idx] + 
                       f_knight_given_loss_count[idx]) / won_or_lost_count;
          double pwf = pfw * pw / pf;

          // Compute the probability p(d|f) excluding wins.
          double drawn_or_lost_count = drawn_positions_count + lost_positions_count;
          double pfd = f_knight_given_draw_count[idx] / drawn_positions_count;
          double pd = drawn_positions_count / drawn_or_lost_count;
          double pf2 = (f_knight_given_draw_count[idx] + 
                        f_knight_given_loss_count[idx]) / drawn_or_lost_count;
          double pdf = pfd * pd / pf2;

          // Compute a weighted average.
          double wa = (2 * pwf + 1 * pdf) / 3;

          // Estimate a material value.
          double mv = round (400 * log10 (wa / (1 - wa)));

          cerr << setw (5);
          cerr << mv;
          cerr << ", ";
        }
      cerr << endl;
    }

  cerr << endl;
}
