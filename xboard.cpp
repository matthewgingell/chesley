////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// xboard.cpp                                                                 //
//                                                                            //
// This file provides Chesley's implementation of the xboard                  //
// interface. The specification for this protocol is available at             //
// http://tim-mann.org/xboard/engine-intf.html.                               //
//                                                                            //
// Copyright Matthew Gingell <matthewgingell@gmail.com>, 2009-2015. Chesley   //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <string>
#include "chesley.hpp"

using namespace std;

// Set xboard protocol mode.
bool
Session::set_xboard_mode (const string_vector &tokens IS_UNUSED) {
  protocol = XBOARD;
  ui_mode = BATCH;

  // Set chatting for ICS.
  fprintf (out, "tellicsnoalias set 1 %s v%s\n",
           ENGINE_ID_STR, VERSION);
  fprintf (out, "tellicsnoalias kibitz Chesley! v%s says hello!\n",
           VERSION);

  return true;
}
