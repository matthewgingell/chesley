/*
   This file provides Chesley's implementation of the xboard
   interface. The specification for this protocol is available at
   http://tim-mann.org/xboard/engine-intf.html.

   Matthew Gingell
   gingell@adacore.com
*/

#include <string>
#include "chesley.hpp"

using namespace std;

// Set xboard protocol mode.
bool
Session::set_xboard_mode (const string_vector &tokens) {
  protocol = XBOARD;
  ui_mode = BATCH;

  // Set chatting for ICS.
  fprintf (out, "tellicsnoalias set 1 %s v%s\n", ENGINE_ID_STR, VERSION_STR);
  fprintf (out, "tellicsnoalias kibitz Chesley! v%s says hello!\n", VERSION_STR);

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

      /**********************/
      /* protover N command */
      /**********************/

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
	  fprintf (out, "feature myname=\"" PROLOGUE "\"");
	  fprintf (out, "feature colors=0\n");
	  fprintf (out, "feature ics=1\n");
	  fprintf (out, "feature name=1\n");
	  fprintf (out, "feature pause=1\n");
	  fprintf (out, "feature done=1\n");
	}

      /**************************************************************/
      /* Assume we are using an up to date version of xboard and do */
      /* not pay attention to replies to the feature command.       */
      /**************************************************************/

      if (token == "accepted" || token == "rejected")
	{
	  // ignored.
	}

      /***************/
      /* new command */
      /***************/

      if (token == "new")
	{
	  board = Board :: startpos ();
	  our_color = BLACK;
	  running = true;
	}

      /******************************************************/
      /* We do not support any non-standard chess variants. */
      /******************************************************/

      if (token == "variant")
	{
	  // ignored.
	}

      /****************/
      /* quit command */
      /****************/

      if (token == "quit")
	{
	  return false;
	}

      /******************/
      /* random command */
      /******************/

      if (token == "random")
	{
	  // ignored.
	}

      /*****************/
      /* force command */
      /*****************/

      if (token == "force")
	{
	  running = false;
	}

      /**************/
      /* go command */
      /**************/

      if (token == "go")
	{
	  our_color = board.flags.to_move;
	  running = true;
	}

      /*********************/
      /* playother command */
      /*********************/

      if (token == "playother")
	{
	  our_color = invert_color (our_color);
	}

      /*****************/
      /* white command */
      /*****************/

      if (token == "white")
	{
	  board.set_color (WHITE);
	  our_color = BLACK;
	}

      /*****************/
      /* black command */
      /*****************/

      if (token == "black")
	{
	  board.set_color (BLACK);
	  our_color = WHITE;
	}

      /******************************/
      /* level MPS BASE INC command */
      /******************************/

      if (token == "level")
	{
	  // ignored.
	}

      /*******************/
      /* st TIME command */
      /*******************/

      if (token == "st")
	{
	  // ignored.
	}

      /******************/
      /* time N command */
      /******************/

      if (token == "time")
	{
	  // ignored.
	}

      /*******************/
      /* otime N command */
      /*******************/

      if (token == "otime")
	{
	  // ignored.
	}

      /********************/
      /* usermove command */
      /********************/

      if (token == "usermove" && count > 1)
	{
	  Move m = board.from_calg (tokens[1]);

	  bool applied = board.apply (m);

	  // The client should never pass us a move that doesn't
	  // apply.
	  assert (applied);

	    // This move may have ended the game.
	  Status s = get_status ();
	  if (s != GAME_IN_PROGRESS)
	    {
	      handle_end_of_game (s);
	    }
	}

      /**************/
      /* ? command. */
      /**************/

      if (token == "?")
	{
	  // ignored.
	}

      /*******************/
      /* ping N command. */
      /*******************/

      if (token == "ping") {
	fprintf (out, "pong");
	if (count > 1)
	  {
	    fprintf (out, " %s", tokens[1].c_str ());
	  }
	fprintf (out, "\n");
      }

      /****************/
      /* draw command */
      /****************/

      if (token == "draw")
	{
	  // ignored.
	}

      /***********************************/
      /* result RESULT {COMMENT} command */
      /***********************************/

      if (token == "result")
	{
	  // ignored.
	}

      /************************/
      /* setboard FEN command */
      /************************/

      if (token == "setboard")
	{
	  // Handled in generic loop.
	}

      /****************/
      /* edit command */
      /****************/

      if (token == "edit")
	{
	  // ignored.
	}

      /****************/
      /* hint command */
      /****************/

      if (token == "hint")
	{
	  // ignored.
	}

      /**************/
      /* bk command */
      /**************/

      if (token == "bk")
	{
	  // ignored.
	}

      /****************/
      /* undo command */
      /****************/

      if (token == "undo")
	{
	  // ignored.
	}

      /******************/
      /* remove command */
      /******************/

      if (token == "remove")
	{
	  // ignored.
	}

      /****************/
      /* hard command */
      /****************/

      if (token == "hard")
	{
	  // ignored.
	}

      /****************/
      /* easy command */
      /****************/

      if (token == "easy")
	{
	  // ignored.
	}

      /****************/
      /* post command */
      /****************/

      if (token == "post")
	{
	  // ignored.
	}

      /******************/
      /* nopost command */
      /******************/

      if (token == "nopost")
	{
	  // ignored.
	}

      /*******************/
      /* analyze command */
      /*******************/

      if (token == "analyse")
	{
	  // ignored.
	}

      /******************/
      /* name X command */
      /******************/

      if (token == "name")
	{
	  // ignored.
	}

      /******************/
      /* rating command */
      /******************/

      if (token == "rating")
	{
	  // ignored.
	}

      /************************/
      /* ics HOSTNAME command */
      /************************/

      if (token == "ics")
	{
	  // ignored.
	}

      /********************/
      /* computer command */
      /********************/

      if (token == "computer")
	{
	  op_is_computer = true;
	}

      /*****************/
      /* pause command */
      /*****************/

      if (token == "pause")
	{
	  // ignored.
	}

      /******************/
      /* resume command */
      /******************/

      if (token == "resume")
	{
	  // ignored.
	}
    }

  return true;
}
