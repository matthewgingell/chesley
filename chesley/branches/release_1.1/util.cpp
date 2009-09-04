////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// util.cpp                                                                   //
//                                                                            //
// Various small utility functions. This file is intended to encapsulate      //
// all the platform dependent functionality used by Chesley.                  //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <boost/random.hpp>

#include "common.hpp"
#include "util.hpp"

///////////////////////////////
// Random number generation  //
///////////////////////////////

static boost::mt19937 generator;
static boost::uniform_int <uint64> distribution (0ULL, ~0ULL); 

// Seed the random number generator.
void seed_random () {
  generator.seed (mclock ());
}

// Return a 64-bit random number.
uint64 random64 () {
  return distribution (generator);
}
