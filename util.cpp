////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// util.cpp                                                                   //
//                                                                            //
// Various small utility functions. This file is intended to encapsulate      //
// all the platform dependent functionality used by Chesley.                  //
//                                                                            //
// Copyright Matthew Gingell <matthewgingell@gmail.com>, 2009-2015. Chesley   //
// the Chess Engine! is free software distributed under the terms of the      //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#if 0

#include "common.hpp"
#include "util.hpp"

///////////////////////////////
// Random number generation  //
///////////////////////////////

// Seed the random number generator.
void seed_random () {
  generator.seed (mclock ());
}

// Return a 64-bit random number.
uint64 random64 () {
  return distribution (generator);
}

#endif
