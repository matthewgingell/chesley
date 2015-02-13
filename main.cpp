////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// main.cpp                                                                   //
//                                                                            //
// Chesley the Chess Engine!                                                  //
//                                                                            //
// Copyright Matthew Gingell <matthewgingell@gmail.com>, 2009-2015. Chesley   //
// the Chess Engine! is free software distributed under the terms of the      //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <string>
#include <cstdio>

#include "chesley.hpp"

using namespace std;

// Globals.
char *arg0;

// Initialization.
void initialize_all ()
 {
   // Set unbuffered I/O mode.
   setvbuf (stdin, NULL, _IONBF, 0);
   setvbuf (stdout, NULL, _IONBF, 0);

   // Build move generation and other miscellaneous tables.
   precompute_tables ();

   // Initialize the user session.
   Session :: init_session ();
 }

// Initialize and pass control to main loop.
int main(int argc IS_UNUSED, char **argv)
{
  arg0 = argv[0];

  try
    {
      initialize_all ();
      Session::cmd_loop ();
    }
  catch (string s)
    {
      cerr << "Caught exception at top level: " << s << endl;
    }

  return 0;
}
