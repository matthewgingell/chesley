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

#include <string>

// Generate statistics about material balance.
void gen_material_stats (const std::string filename); 

// Generate piece square tables from a .pgn file.
void gen_psq_tables (const std::string filename);

#endif // _STATS_
