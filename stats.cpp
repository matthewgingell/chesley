////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// stats.hpp                                                                  //
//                                                                            //
// Code to analyze chess games and collect statistics.                        //
//                                                                            //
// Copyright Matthew Gingell <matthewgingell@gmail.com>, 2009-2015.           //
// Chesley the Chess Engine! is free software distributed under the terms of  //
// the GNU Public License.                                                    //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>

#include "chesley.hpp"

// MSVC doesn't provide an isnan in <cmath>.
#if defined(_MSC_VER) && !defined(isnan)
inline bool isnan(double v) { return v!=v; }
#endif

using namespace std;

/////////////////////////////////////////////////////////////////////////
//                                                                     //
// struct Table                                                        //
//                                                                     //
// Simulate an infinite array of real numbers initialized to zero with //
// a domain over the positive and negative integers.                   //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

struct Table;
ostream & operator<< (ostream &os, const Table &o);
typedef map <int64, double> sarray;

struct Table {

  // Update or insert an element.
  void set (int64 index, double value) {
    if (value == 0) return;
    sarray :: iterator i = elements.find (index);
    if (i != elements.end()) i -> second = value;
    else elements.insert (pair <int64, double> (index, value));
  }

  // Fetch an element.
  double get (int64 index) {
    sarray :: iterator i = elements.find (index);
    if (i == elements.end ()) return 0;
    else return i -> second;
  }

  // Increment an element.
  void inc (int64 index) {
    double old = get (index);
    set (index, old + 1);
  }

  // Index of first non-zero element.
  int64 first () {
    sarray :: iterator i = elements.begin ();
    if (i == elements.end ()) return -1;
    else return i -> first;
  }

  // Index of last non-zero element.
  int64 last () {
    sarray :: reverse_iterator i = elements.rbegin ();
    if (i == elements.rend ()) return 1;
    else return i -> first;
  }

  // Count the number of elements in the table.
  int64 count () {
    return elements.size ();
  }

  // Apply Good-Turing smoothing to the vector.
  void smooth () {
    Table Zr;
    sarray :: iterator i;

    // Compute frequency of frequencies.
    for (i = elements.begin (); i != elements.end (); i++)
      Zr.inc ((int64) i -> second);

    // Smooth the zeros in Z.
    Zr.smooth_zeros ();

    // Compute the coefficients of a log-log regression of the
    // frequency of frequencies.
    double a, b;
    Zr.regress (a, b);

    // Smooth the values.
    for (i = elements.begin (); i != elements.end (); i++)
      {
        double t = i -> second;
        i -> second = t * pow (1 + 1 / t, b + 1);
      }
  }

  // Average every element with the zeros which surround it.
  void smooth_zeros () {
    sarray z;
    sarray :: const_iterator r, t, q;
    uint64 width;

    if (elements.size () <= 1)
      return;

    q = r = t = elements.begin ();
    t++;

    // Handle the first element.
    width = (t -> first) - (r -> first);
    z[r -> first] = r -> second / width;

    r++; t++;
    while (true)
      {
        // Handle the last element.
        if (t == elements.end ())
          {
            width = (r -> first) - (q -> first);
            z[r -> first] = r -> second / width;
            break;
          }

        // Handle the inner elements.
        width = (t -> first) - (q -> first);
        z[r -> first] = 2 * r -> second / width;

        q++; r++; t++;
      }

    elements = z;
  }

  // Compute coefficients for a log-log linear regression.
  void regress (double &a, double &b) {
    sarray :: const_iterator i;
    int count = 0;

    // Compute the average values.
    double mean_x = 0, mean_y = 0;
    for (i = elements.begin (); i != elements.end (); i++)
      {
        count++;
        mean_x += log ((double) i -> first);
        mean_y += log ((double) i -> second);
      }

    mean_x /= count;
    mean_y /= count;

    // Compute sigma_xy and sigma_x.
    double sigma_xy = 0, sigma_xx = 0;
    for (i = elements.begin (); i != elements.end (); i++)
      {
        double x = i -> first, y = i -> second;
        sigma_xy += (log (x) - mean_x) * (log (y) - mean_y);
        sigma_xx += (log (x) - mean_x) * (log (x) - mean_x);
      }

    b = sigma_xy / sigma_xx;
    a = mean_y - b * mean_x;
  }

  sarray elements;
};

// Output a struct Table to a stream.
ostream & operator<< (ostream &os, const Table &t) {
  sarray :: const_iterator i;
  for (i = t.elements.begin (); i != t.elements.end (); i++)
    {
      os << setiosflags (ios::fixed);
      os << setprecision (4);
      os << setw (9) << i -> first;
      os << setw (9) << i -> second;
      os << endl;
    }
  return os;
}

/////////////////////////////////////////////////////////////////
// Count up the number of wins, loses, and draws over a set of //
// games.                                                      //
/////////////////////////////////////////////////////////////////

void
gen_material_stats (const string filename)
{
  Table wins, losses, draws;

  PGN pgn;
  pgn.open (filename.c_str());
  while (true)
    {
      Game g = pgn.read_game ();
      if (pgn.status == PGN::END_OF_FILE) break;
      if (g.winner == NULL_COLOR) continue;

      Board b = Board::startpos ();
      int stable_count = 0;
      for (uint32 i = 0; i < g.moves.size (); i++)
        {
          if (g.moves[i].get_capture () == NULL_KIND)
            {
              stable_count++;
            }
          else
            {
              stable_count = 0;
            }

          if (stable_count >= 5)
            {
              // Collect material balance information.
              Score mdif = b.material[WHITE] - b.material[BLACK];

              if (g.winner == WHITE)
                {
                  wins.inc (mdif);
                  losses.inc (-mdif);
                }
              else if (g.winner == BLACK)
                {
                  wins.inc (-mdif);
                  losses.inc (mdif);
                }
              else if (g.winner == NULL_COLOR)
                {
                  draws.inc (mdif);
                  draws.inc (-mdif);
                }
            }

          if (!b.apply (g.moves[i])) break;
        }
    }

  wins.smooth ();
  losses.smooth ();
  draws.smooth ();
  for (int i = -200; i <= 200; i++)
    {
      double nwins = wins.get (i);
      double npositions = wins.get (i) + losses.get (i) + draws.get (i);

      if (npositions > 0 && nwins > 0)
        {
          cout << setiosflags (ios::fixed);
          cout << setprecision (4);
          cout << setw (10) << i;
          cout << setw (10) << nwins / npositions;
          cout << endl;
        }
    }

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

const double b0 = 0;
const double b1 = 0.01182;

double probability_to_material (double p) {
  return log ((p / (1 - p)) - b0) / b1;
}

struct psq_generator {

  psq_generator (const char *filename) {
    npos = nwins = nlosses = ndraws = 0;
    ZERO (ff);
    ZERO (ffw);
    ZERO (ffd);
    ZERO (ffl);
    pgn.open (filename);
  }

  void output () {
    collect_positions ();
    cout << "const int Eval::piece_square_table[6][64] =" << endl;
    cout << "{" << endl;
    for (Kind k = PAWN; k <= KING; k++)
      {
        output_feature (k);
        if (k != KING) cout << "," << endl;
        cout << endl;
      }
    cout << "};" << endl;
  }

private:

  // Collect data over every position in a PGN stream.
  void collect_positions () {
    Game g;
    while (true)
      {
        g = pgn.read_game ();
        if (pgn.status == PGN::END_OF_FILE) break;

        Board b = Board::startpos ();
        for (uint32 i = 0; i < g.moves.size (); i++)
          {
            collect_features (b, g);
            if (!b.apply (g.moves[i])) break;
          }
      }
  }

  // Collect features of a position.
  void collect_features (const Board &b, Game g) {

    //    if (abs(b.material[WHITE] - b.material[BLACK]) > 0)
    //      return;

    // Assume left-right symmetry.
    npos += 2;
    if (g.winner == WHITE) nwins += 2;
    if (g.winner == BLACK) nlosses += 2;
    if (g.winner == NULL_COLOR) ndraws += 2;

    for (Kind k = PAWN; k <= KING; k++)
      {
        bitboard pieces = b.kind_to_board (k) & b.white;
        while (pieces)
          {
            Coord idx = bit_idx (pieces);
            Coord flip = flip_left_right[idx];
            ff[k][idx]++;
            ff[k][flip]++;
            if (g.winner == WHITE)
              {
                ffw[k][idx]++;
                ffw[k][flip]++;
              }
            else if (g.winner == BLACK)
              {
                ffl[k][idx]++;
                ffl[k][flip]++;
              }
            else if (g.winner == NULL_COLOR)
              {
                ffd[k][idx]++;
                ffd[k][flip]++;
              }

            clear_bit (pieces, idx);
          }

        // Assume black-white symmetry.
        npos += 2;
        if (g.winner == BLACK) nwins += 2;
        if (g.winner == WHITE) nlosses += 2;
        if (g.winner == NULL_COLOR) ndraws += 2;

        pieces = b.kind_to_board (k) & b.black;
        while (pieces)
          {
            Coord idx = flip_white_black[bit_idx (pieces)];
            Coord flip = flip_left_right[idx];
            ff[k][idx]++;
            ff[k][flip]++;
            if (g.winner == BLACK)
              {
                ffw[k][idx]++;
                ffw[k][flip]++;
              }
            else if (g.winner == WHITE)
            {
              ffl[k][idx]++;
              ffl[k][flip]++;
            }
            else if (g.winner == NULL_COLOR)
              {
                ffd[k][idx]++;
                ffd[k][flip]++;
              }

            clear_bit (pieces, idx);
          }
      }

  }

  void output_feature (Kind k)
  {
    cout << "  // " << k << endl;
    cout << "  { " << endl;
    for (int rank = 7; rank >= 0; rank--)
      {
        cout << "  ";
        for (int file = 0; file <= 7; file++)
          {
            int idx = to_idx (rank, file);

            // Apply Bayes law to find the material value of this
            // position as a probability of winning.
            double pfw = ffw[k][idx] / nwins;
            double pw = .5;
            double pf = ff[k][idx] / (npos);
            double pwf = pfw * pw / pf;
            double mwf = probability_to_material (pwf);
            if (isnan (mwf)) mwf = 0;

            // Apply Bayes law to find the material value of this
            // position as a probability of drawing.
            double pfd = ffd[k][idx] / ndraws;
            double pd = .5;
            double pdf = pfd * pd / pf;
            double mdf = probability_to_material (pdf);
            if (isnan (mdf)) mdf = 0;

            // The material value is a weighted average of the values
            // for win and draw.
            double m = (2 * mwf + mdf) / 3;

            // Output this table entry.
            cout << setiosflags (ios::fixed);
            cout << setprecision (0);
            cout << setw (5);
            if (isnan (m)) m = 0;
            cout << m;
            if (rank != 0 || file != 7) cout << ",";
          }
        cout << endl;
      }
    cout << "  }";
  }

  // Stream of games.
  PGN pgn;

  // Absolute frequencies of features.
  double npos, nwins, nlosses, ndraws;
  double ff[KIND_COUNT][64];
  double ffw[KIND_COUNT][64];
  double ffl[KIND_COUNT][64];
  double ffd[KIND_COUNT][64];
};

void gen_psq_tables (const string filename) {
  psq_generator psqg (filename.c_str());
  psqg.output ();
  return;
}
