/*
   Chesley the chess engine! 

   Matthew Gingell
   gingell@gnat.com
*/

#include "chesley.hpp"

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
  initialize_all ();
  Session::cmd_loop ();
  return 0;
}
