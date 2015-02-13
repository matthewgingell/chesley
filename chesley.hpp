////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// chesley.hpp                                                                //
//                                                                            //
// Chesley the Chess Engine!                                                  //
//                                                                            //
// Copyright Matthew Gingell <matthewgingell@gmail.com>, 2009-2015. Chesley   //
// the Chess Engine! is free software distributed under the terms of the      //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef __CHESLEY__
#define __CHESLEY__

#include <cstdio>

#include "bits64.hpp"
#include "board.hpp"
#include "common.hpp"
#include "eval.hpp"
#include "move.hpp"
#include "pgn.hpp"
#include "phash.hpp"
#include "search.hpp"
#include "session.hpp"
#include "stats.hpp"
#include "ttable.hpp"
#include "util.hpp"

#define ENGINE_ID_STR        "Chesley the Chess Engine!"
#define ENGINE_AUTHOR_STR    "Matthew Gingell <gingell@gmail.com>"
#define ENGINE_COPYRIGHT_STR "Copyright (C) 2015 " ENGINE_AUTHOR_STR
#define ENGINE_LICENSE_STR   "Chesley is free software, " \
                              "distributed under the GNU General Public License."
#define VERSION "1.2.x"

inline
char *get_prologue () {
  static char buf[2048];
  snprintf (buf, sizeof (buf), "%s (%s)\n\n%s\n%s",
           ENGINE_ID_STR, VERSION, ENGINE_COPYRIGHT_STR, ENGINE_LICENSE_STR);
  return buf;
}

#endif // __CHESLEY__
