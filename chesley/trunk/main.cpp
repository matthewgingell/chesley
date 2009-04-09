////////////////////////////////////////////////////////////////////////////////
// 								     	      //
// main.cpp							     	      //
// 								     	      //
// Chesley the Chess Engine!                                       	      //
// 								     	      //
// Matthew Gingell						     	      //
// gingell@adacore.com					     		      //
// 								     	      //
////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <string>

#include "chesley.hpp"

using namespace std;

// Initializion
void initialize_all ()
{
  seed_random ();
  Board :: precompute_tables ();
  Session :: init_session ();
}

// Initialize and pass control to main loop.
int main()
{
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
