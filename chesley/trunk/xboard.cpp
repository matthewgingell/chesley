////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// xboard.cpp                                                                 //
//                                                                            //
// This file provides Chesley's implementation of the xboard                  //
// interface. The specification for this protocol is available at             //
// http://tim-mann.org/xboard/engine-intf.html.                               //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
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
           ENGINE_ID_STR, SVN_REVISION);
  fprintf (out, "tellicsnoalias kibitz Chesley! v%s says hello!\n",
           SVN_REVISION);

  return true;
}

// Execute a command in xboard mode.
bool
Session::xbd_execute (char *line) {
  string_vector tokens = tokenize (line);
  int count = tokens.size ();

  if (tokens.size () > 0)
    {
      string token = downcase (tokens[0]);

      ////////////////////////
      // protover N command //
      ////////////////////////

      if (token == "protover")
        {
          // Send xboard the list of features we support.
          fprintf (out, "feature ping=1\n");
          fprintf (out, "feature setboard=1\n");
          fprintf (out, "feature playother=1\n");
          fprintf (out, "feature san=0\n");
          fprintf (out, "feature usermove=1\n");
          fprintf (out, "feature time=1\n");
          fprintf (out, "feature draw=1\n");
          fprintf (out, "feature sigint=0\n");
          fprintf (out, "feature sigterm=1\n");
          fprintf (out, "feature reuse=1\n");
          fprintf (out, "feature analyze=0\n");
          //      fprintf (out, "feature myname=\"%s\"", get_prologue ());
          fprintf (out, "feature myname=\"%s\"", arg0);
          fprintf (out, "feature colors=0\n");
          fprintf (out, "feature ics=1\n");
          fprintf (out, "feature name=1\n");
          fprintf (out, "feature pause=1\n");
          fprintf (out, "feature done=1\n");
        }


      // Assume we are using an up to date version of xboard and do
      // not pay attention to replies to the feature command.
      else if (token == "accepted" || token == "rejected")
        {
          // ignored.
        }


      // We do not support any non-standard chess variants.
      else if (token == "variant")
        {
          // ignored.
        }

      ////////////////////
      // random command //
      ////////////////////

      if (token == "random")
        {
          // ignored.
        }

      ////////////////
      // ? command. //
      ////////////////

      if (token == "?")
        {
          // ignored.
        }

      /////////////////////
      // ping N command. //
      /////////////////////

      if (token == "ping") {
        fprintf (out, "pong");
        if (count > 1)
          {
            fprintf (out, " %s", tokens[1].c_str ());
          }
        fprintf (out, "\n");
      }

      //////////////////
      // draw command //
      //////////////////

      if (token == "draw")
        {
          // ignored.
        }

      /////////////////////////////////////
      // result RESULT {COMMENT} command //
      /////////////////////////////////////

      if (token == "result")
        {
          // ignored.
        }

      //////////////////
      // edit command //
      //////////////////

      if (token == "edit")
        {
          // ignored.
        }

      //////////////////
      // hint command //
      //////////////////

      if (token == "hint")
        {
          // ignored.
        }

      ////////////////
      // bk command //
      ////////////////

      if (token == "bk")
        {
          // ignored.
        }

      //////////////////
      // undo command //
      //////////////////

      if (token == "undo")
        {
          // ignored.
        }

      ////////////////////
      // remove command //
      ////////////////////

      if (token == "remove")
        {
          // ignored.
        }

      //////////////////
      // hard command //
      //////////////////

      if (token == "hard")
        {
          // ignored.
        }

      //////////////////
      // easy command //
      //////////////////

      if (token == "easy")
        {
          // ignored.
        }

      //////////////////
      // post command //
      //////////////////

      if (token == "post")
        {
          // ignored.
        }

      ////////////////////
      // nopost command //
      ////////////////////

      if (token == "nopost")
        {
          // ignored.
        }

      /////////////////////
      // analyze command //
      /////////////////////

      if (token == "analyze")
        {
          // ignored.
        }

      ////////////////////
      // name X command //
      ////////////////////

      if (token == "name")
        {
          // ignored.
        }

      ////////////////////
      // rating command //
      ////////////////////

      if (token == "rating")
        {
          // ignored.
        }

      //////////////////////////
      // ics HOSTNAME command //
      //////////////////////////

      if (token == "ics")
        {
          // ignored.
        }

      //////////////////////
      // computer command //
      //////////////////////

      if (token == "computer")
        {
          op_is_computer = true;
        }

      ///////////////////
      // pause command //
      ///////////////////

      if (token == "pause")
        {
          // ignored.
        }

      ////////////////////
      // resume command //
      ////////////////////

      if (token == "resume")
        {
          // ignored.
        }
    }

  return true;
}
