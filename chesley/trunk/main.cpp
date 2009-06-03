////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// main.cpp                                                                   //
//                                                                            //
// Chesley the Chess Engine!                                                  //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <string>

#include "chesley.hpp"

using namespace std;

// Globals.
char *arg0;

// Initialization.
void initialize_all ()
{
  seed_random ();
  Board :: precompute_tables ();
  Session :: init_session ();
}

// Initialize and pass control to main loop.
int main(int argc, char **argv)
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
