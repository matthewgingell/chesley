/*
   Chesley the chess engine! 

   Matthew Gingell
   gingell@gnat.com
*/

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

extern char *SVN_REVISION;

#define ENGINE_ID_STR     "Chesley!"
#define ENGINE_AUTHOR_STR "Matthew Gingell"

inline 
char *get_prologue () {
  static char buf[1024];
  sprintf (buf, "%s r%s", ENGINE_ID_STR, SVN_REVISION);
  return buf;
}

#endif // __CHESLEY__
