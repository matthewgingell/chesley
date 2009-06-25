////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// chesley.hpp                                                                //
//                                                                            //
// Chesley the Chess Engine!                                                  //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef __CHESLEY__
#define __CHESLEY__

#include <cstdio>

#include "bits64.hpp"
#include "board.hpp"
#include "eval.hpp"
#include "search.hpp"
#include "session.hpp"
#include "types.hpp"
#include "util.hpp"

#define ENGINE_ID_STR        "Chesley the Chess Engine!"
#define ENGINE_AUTHOR_STR    "Matthew Gingell <gingell@adacore.com>"
#define ENGINE_COPYRIGHT_STR "Copyright (C) 2009 " ENGINE_AUTHOR_STR
#define ENGINE_LICENSE_STR   "Chesley is free software, " \
                              "distributed under the GNU General Public License."

#ifdef RELEASE_VERSION
#define VSN_STRING RELEASE_VERSION
#else
#define VSN_STRING "<unknown release>"
#endif


inline
char *get_prologue () {
  static char buf[2048];
  sprintf (buf, "%s (Release %s)\n\n%s\n%s",
           ENGINE_ID_STR, VSN_STRING, ENGINE_COPYRIGHT_STR, ENGINE_LICENSE_STR);
  return buf;
}

#endif // __CHESLEY__
