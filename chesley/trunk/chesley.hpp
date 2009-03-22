/*
   Chesley the chess engine! 

   Matthew Gingell
   gingell@gnat.com
*/

#ifndef __CHESLEY__
#define __CHESLEY__

#include "vsn.inc"

#include "bits64.hpp"
#include "board.hpp"
#include "eval.hpp"
#include "search.hpp"
#include "session.hpp"
#include "types.hpp"
#include "util.hpp"

#define ENGINE_ID_STR     "Chesley!"
#define VERSION_STR       SVN_REVISION
#define ENGINE_AUTHOR_STR "Matthew Gingell"
#define PROLOGUE          "Chesley! r" SVN_REVISION "\n"

#endif // __CHESLEY__
