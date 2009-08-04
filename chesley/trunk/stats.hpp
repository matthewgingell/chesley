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

#ifndef _STATS_
#define _STATS_

// Output four columns: The first is Chesley's eval, followed by win
// count, loss count, and draw count.
void collect_eval_vs_winp  (const char *filename);

// Generate piece square tables from a .pgn file.
void gen_psq_tables (const char *filename);

#endif // _STATS_

