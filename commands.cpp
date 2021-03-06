////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// commands.cpp                                                               //
//                                                                            //
// Code for executing commands passed to the command interpreter.             //
//                                                                            //
// Copyright Matthew Gingell <matthewgingell@gmail.com>, 2009-2015. Chesley   //
// the Chess Engine! is free software distributed under the terms of the      //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <cstdio>
#include <iostream>
#include <iomanip>
#include <string>

#include "chesley.hpp"
#include "pgn.hpp"
#include "stats.hpp"

using namespace std;

//////////////////////////
//                      //
// Command declarations //
//                      //
//////////////////////////

enum Command_Kind
  {
    USER_CMD, DEBUG_CMD, STATS_CMD, XBOARD_CMD
  };

enum Command
  {
    CMD_NULL = -1,

    ///////////////////
    // User commands //
    ///////////////////

    CMD_BLACK,
    CMD_DISP,
    CMD_DTC,
    CMD_EVAL,
    CMD_FEN,
    CMD_FORCE,
    CMD_GO,
    CMD_HELP,
    CMD_INTERRUPT,
    CMD_LEVEL,
    CMD_MOVE,
    CMD_MOVES,
    CMD_NEW,
    CMD_PLAYOTHER,
    CMD_PLAYSELF,
    CMD_QUIT,
    CMD_SD,
    CMD_SETBOARD,
    CMD_ST,
    CMD_TIME,
    CMD_WHITE,

    ////////////////////////
    // Debugging commands //
    ////////////////////////

    CMD_APPLY,
    CMD_ATTACKS,
    CMD_BENCH,
    CMD_DIV,
    CMD_DUMPPAWNS,
    CMD_DUMPPGN,
    CMD_EPD,
    CMD_HASH,
    CMD_PERFT,
    CMD_TESTHASHING,

    ///////////////////////////
    // Statistics collection //
    ///////////////////////////

    CMD_GENMSTATS,
    CMD_GENPSQ,

    /////////////////////
    // XBoard commands //
    /////////////////////

    CMD_ACCEPTED,
    CMD_ANALYZE,
    CMD_BK,
    CMD_COMPUTER,
    CMD_DRAW,
    CMD_EASY,
    CMD_EDIT,
    CMD_HARD,
    CMD_HINT,
    CMD_ICS,
    CMD_NAME,
    CMD_NOPOST,
    CMD_OTIM,
    CMD_PAUSE,
    CMD_PING,
    CMD_POST,
    CMD_PROTOVER,
    CMD_QMARK,
    CMD_RANDOM,
    CMD_RATING,
    CMD_REJECTED,
    CMD_REMOVE,
    CMD_RESULT,
    CMD_RESUME,
    CMD_USERMOVE,
    CMD_VARIANT,
    CMD_XBOARD,

    CMD_COUNT
  };

static const struct {
  Command      code;
  Command_Kind kind;
  char const  *str;
  char const  *usage;
  char const  *doc;
} commands[CMD_COUNT] = {


  ///////////////////
  // User commands //
  ///////////////////

  { CMD_BLACK, USER_CMD,      "BLACK",     "",
    "Set user to play black." },

  { CMD_DISP,  USER_CMD,      "DISP",      "",
    "Print this position." },

  { CMD_DTC,   USER_CMD,      "DTC" ,      "",
    "Print time control settings." },

  { CMD_EVAL,  USER_CMD,      "EVAL",      "",
    "Print the static evaluation for this position."},

  { CMD_FEN,   USER_CMD,      "FEN",       "",
    "Print a FEN string for this position."},

  { CMD_FORCE, USER_CMD,      "FORCE",     "",
    "Freeze the engine." },

  { CMD_GO,    USER_CMD,      "GO",        "",
    "Set the engine running."},

  { CMD_HELP,  USER_CMD,      "HELP",      "",
    "Print a help message."},

  { CMD_INTERRUPT,  USER_CMD, "?",         "",
    "Interrupt the search and return to the command time."},

  { CMD_LEVEL, USER_CMD,      "LEVEL",     "<moves time increment>",
    "Set time controls"},

  { CMD_MOVE,  USER_CMD,      "MOVE",      "<move>",
    "Make a move." },

  { CMD_MOVES, USER_CMD,      "MOVES",     "",
    "Print a list of all legal moves." },

  { CMD_NEW,   USER_CMD,      "NEW",       "",
    "Start a new game." },

  { CMD_PLAYOTHER, USER_CMD,  "PLAYOTHER", "",
    "Swap the sides played by the engine and the user." },

  { CMD_PLAYSELF, USER_CMD,   "PLAYSELF",  "",
    "Display play a computer vs. computer games."},

  { CMD_QUIT,  USER_CMD,      "QUIT",      "",
    "Quit Chesley." },

  { CMD_SD,    USER_CMD,       "SD",       "",
    "Set a fixed search depth limit."},

  { CMD_SETBOARD, USER_CMD,   "SETBOARD",  "<fen>",
    "Set the board from a FEN string."},

  { CMD_ST,    USER_CMD,      "ST",        "<time>",
    "Set a fixed time per move"},

  { CMD_TIME,  USER_CMD,      "TIME",      "<centiseconds>",
    "Set time remaining on the clock."},

  { CMD_WHITE, USER_CMD,      "WHITE",     "",
    "Set user to play black." },

  ////////////////////////
  // Debugging commands //
  ////////////////////////

  { CMD_APPLY,      DEBUG_CMD,     "APPLY",     "",
    "Apply a move." },

  { CMD_ATTACKS,    DEBUG_CMD,     "ATTACKS",   "",
    "Display a map of attacked squares." },

  { CMD_BENCH,      DEBUG_CMD,     "BENCH",     "<depth>",
    "Analyze this position to a fixed depth." },

  { CMD_DIV,        DEBUG_CMD,     "DIV",       "<depth>",
    "Compute div to a fixed depth" },

  { CMD_DUMPPAWNS,  DEBUG_CMD,     "DUMPPAWNS", "",
    "Dump a vector of pawns."
  },

  { CMD_DUMPPGN,    DEBUG_CMD,     "DUMPPGN", "",
    "Read and dump a PGN file."
  },

  { CMD_EPD,        DEBUG_CMD,     "EPD",       "<epd>",
    "Evaluate an EPD string."},

  { CMD_HASH,       DEBUG_CMD,     "HASH",      "",
    "Print the current position hash."},

  { CMD_PERFT,      DEBUG_CMD,     "PERFT",     "<depth>",
    "Compute perft to a fixed depth."},

  { CMD_TESTHASHING,DEBUG_CMD, "TESTHASHING", "",
    "Run a test on hash code generation."},

  ///////////////////////////
  // Statistics collection //
  ///////////////////////////

  { CMD_GENMSTATS, STATS_CMD, "GENMSTATS", "",
    "Generate statistics about material balance." },

  { CMD_GENPSQ, STATS_CMD, "GENPSQ", "",
    "Generate piece square tables from a .pgn file."},

  /////////////////////
  // XBoard commands //
  /////////////////////

  { CMD_ACCEPTED, XBOARD_CMD, "ACCEPTED",  "",
    ""},

  { CMD_ANALYZE,  XBOARD_CMD, "ANALYZE",   "",
    ""},

  { CMD_BK,       XBOARD_CMD, "BK",        "",
    ""},

  { CMD_COMPUTER, XBOARD_CMD, "COMPUTER",  "",
    ""},

  { CMD_DRAW,     XBOARD_CMD, "DRAW",      "",
    ""},

  { CMD_EASY,     XBOARD_CMD, "EASY",      "",
    ""},

  { CMD_EDIT,     XBOARD_CMD, "EDIT",      "",
    ""},

  { CMD_HARD,     XBOARD_CMD, "HARD",      "",
    ""},

  { CMD_HINT,     XBOARD_CMD, "HINT",      "",
    ""},

  { CMD_ICS,      XBOARD_CMD,  "ICS",       "",
    ""},

  { CMD_NAME,     XBOARD_CMD,  "NAME",      "",
    ""},

  { CMD_NOPOST,   XBOARD_CMD,  "NOPOST",    "",
    ""},

  { CMD_OTIM,     XBOARD_CMD,  "OTIM",      "",
    "Command ignored."},

  { CMD_PAUSE,    XBOARD_CMD,  "PAUSE",     "",
    ""},

  { CMD_PING,     XBOARD_CMD,  "PING",      "",
    ""},

  { CMD_POST,     XBOARD_CMD,  "POST",      "",
    ""},

  { CMD_PROTOVER, XBOARD_CMD,  "PROTOVER",  "",
    ""},

  { CMD_QMARK,    XBOARD_CMD,  "QMARK",     "",
    ""},

  { CMD_RANDOM,   XBOARD_CMD,  "RANDOM",    "",
    ""},

  { CMD_RATING,   XBOARD_CMD,  "RATING",    "",
    ""},

  { CMD_REJECTED, XBOARD_CMD,  "REJECTED",  "",
    ""},

  { CMD_REMOVE,   XBOARD_CMD,  "REMOVE",    "",
    ""},

  { CMD_RESULT,   XBOARD_CMD,  "RESULT",    "",
    ""},

  { CMD_RESUME,   XBOARD_CMD,  "RESUME",    "",
    ""},

  { CMD_USERMOVE, XBOARD_CMD,  "USERMOVE",  "<move>",
    "Make a move." },

  { CMD_VARIANT,  XBOARD_CMD,  "VARIANT",   "",
    ""},

  { CMD_XBOARD,   XBOARD_CMD,  "XBOARD",    "",
    "Put Chesley in Xboard mode."}
};

// Return a command code from a given string.
static Command match_command (string s);
static Command match_command (string s) {
  s = upcase (s);
  for (int i = 0; i < CMD_COUNT; i++)
    {
      assert (commands[i].code == i);
      if (s == commands[i].str)
        {
          return commands[i].code;
        }
    }
  return CMD_NULL;
}

bool
Session::execute (char *line) {

  string_vector tokens = tokenize (line);

  if (tokens.size () == 0) return true;

  string token = tokens[0];
  Command cmd = match_command (token);
  switch (cmd)
    {
    case CMD_NULL:
      fprintf (out, "Unrecognized command: %s\n", token.c_str ());
      break;

    ///////////////////
    // User commands //
    ///////////////////

    case CMD_BLACK:
      // Set user to play black.
      board.set_color (BLACK);
      our_color = WHITE;
      break;

    case CMD_DISP:
      // Write an ASCII art board.
      fprintf (out, "%s\n", (board.to_ascii ()).c_str ());
      break;

    case CMD_DTC:
      // Display the current time controls.
      display_time_controls (rest (tokens));
      break;

    case CMD_EVAL:
      // Output the static evaluation for this position.
      fprintf (out, "%i\n", Eval (board).score ());
      break;

    case CMD_FEN:
      // Write the current position as a fen string.
      fprintf (out, "%s\n", (board.to_fen ()).c_str ());
      break;

    case CMD_FORCE:
      // Put the engine into force mode.
      running = false;
      break;

    case CMD_GO:
      // Leave force mode.
      our_color = board.to_move ();
      running = true;
      break;

    case CMD_HELP:
      // Print help message for the user.
      display_help (rest (tokens));
      break;

    case CMD_INTERRUPT:
      // There is nothing to do here, since we will interrupt on any
      // input.
      break;

    case CMD_LEVEL:
      level (rest (tokens));
      break;

    case CMD_USERMOVE:
    case CMD_MOVE:
      // Play a move.
      {
        Move m = board.from_calg (tokens[1]);
        bool applied = board.apply (m);
        se.rt_push (board);

        // The client should never pass us a move that doesn't
        // apply.
        assert (applied);

        // This move may have ended the game.
        Status s = get_status (board);
        if (s != GAME_IN_PROGRESS)
          {
            handle_end_of_game (s);
          }
      }
      break;

    case CMD_MOVES:
      // Output the legal moves from this position.
      {
        Move_Vector moves (board);
        for (int i = 0; i < moves.count; i++)
          cout << board.to_san (moves[i]) << endl;
      }
      break;

    case CMD_NEW:
      // Start a new game.
      board = Board :: startpos ();
      se.reset ();
      pv.clear ();
      our_color = BLACK;
      running = true;
      break;

    case CMD_PLAYOTHER:
      // Swap colors between the engine and the user.
      our_color = invert (our_color);
      break;

    case CMD_PLAYSELF:
      // Play an engine vs. engine game on the console.
      play_self (tokens);
      break;

    case CMD_QUIT:
      return false;
      break;

    case CMD_SD:
      // Set fixed depth search mode.
      se.set_fixed_depth (to_int (tokens[1]));
      break;

    case CMD_SETBOARD:
      // Set the board from a fen string.
      board = Board::from_fen (rest (tokens), false);
      break;

    case CMD_ST:
      // Set fixed time move mode.
      se.set_fixed_time (1000 * to_int (tokens[1]));
      break;

    case CMD_TIME:
      // Set the clock.
      se.set_time_remaining (10 * to_int (tokens[1]));
      break;

    case CMD_WHITE:
      // Set user to play white.
      board.set_color (WHITE);
      our_color = BLACK;
      break;

    ////////////////////////
    // Debugging commands //
    ////////////////////////

    case CMD_APPLY:

      // Apply list of moves to the current position.

      running = false;
      for (uint32 i = 1; i < tokens.size (); i++)
        {
          Move m = board.from_san (tokens[i]);
          cerr << m << endl;
          board.apply (m);
        }

      break;
      board.apply (board.from_calg (tokens[1]));
      break;

    case CMD_ATTACKS:
      // Write a bitboard of attacks against the side to move.
      print_board (board.attack_set (invert (board.to_move ())));
      break;

    case CMD_BENCH:
      // Search the current position to a fixed depth.
      bench (tokens);
      break;

    case CMD_DIV:
      // Output the perft score for each child.
      board.divide (to_int (tokens[1]));
      break;

    case CMD_DUMPPAWNS:
      // Dump pawn structure to a file.
      dump_pawns (tokens);
      break;

    case CMD_DUMPPGN:
      {
        PGN pgn;
        Game g;
        pgn.open ("all.pgn");
        while (true)
          {
            cout << "Reading a game..." << endl;
            cout << g.metadata["Event"];
            g = pgn.read_game ();
            if (pgn.status == PGN::END_OF_FILE)
              {
                cout << "Null game." << endl;
                break;
              }
            else
              {
                //                cout << g.metadata["Event"] << endl;
              }
            cout << "Finished a game." << endl << endl;
          }
        pgn.close ();
      }
      break;

    case CMD_EPD:
      // Execute an epd string.
      epd (tokens);
      break;

    case CMD_PERFT:
      // Compute perft to a fixed depth.
      {
        int depth = (to_int (tokens[1]));
        for (int i = 1; i <= depth; i++)
          {
            uint64 start = cpu_time();
            uint64 count = board.perft (i);
            uint64 elapsed = cpu_time() - start;
            fprintf (out, "perft (%i) = %9llu, ", i, count);
            fprintf (out, "%5.2f seconds\n", ((double) elapsed) / 1000.0);
          }
      }
      break;

    case CMD_HASH:
      // Output the hash key for this position.
      cout << board.hash << endl;
      break;

    case CMD_TESTHASHING:
      // Check that incrementally update hash codes are identical to
      // codes generated from scratch to depth 5.
      test_hashing (5);
      break;

    ///////////////////////////
    // Statistics collection //
    ///////////////////////////

    case CMD_GENMSTATS:
      // Generate statistics about material balance.
      if (tokens.size () >= 2)
        gen_material_stats (tokens[1]);
      break;

    case CMD_GENPSQ:
      // Generate piece square tables from a .pgn file.
      if (tokens.size () >= 2)
        gen_psq_tables (tokens[1]);
      break;

    /////////////////////
    // XBoard commands //
    /////////////////////

    case CMD_ACCEPTED:
      // ignored.
      break;

    case CMD_ANALYZE:
      // ignored.
      break;

    case CMD_BK:
      // ignored.
      break;

    case CMD_COMPUTER:
      // We have been informed the opponent is a computer.
      op_is_computer = true;
      break;

    case CMD_DRAW:
      // ignored.
      break;

    case CMD_EASY:
      Session::ponder_enabled = false;
      break;

    case CMD_EDIT:
      // ignored.
      break;

    case CMD_HARD:
      Session::ponder_enabled = true;
      break;

    case CMD_HINT:
      // ignored.
      break;

    case CMD_ICS:
      // ignored.
      break;

    case CMD_NAME:
      // ignored.
      break;

    case CMD_OTIM:
      // ignored.
      break;

    case CMD_NOPOST:
      // ignored.
      break;

    case CMD_PAUSE:
      // ignored.
      break;

    case CMD_PING:
      fprintf (out, "pong %s\n", tokens[1].c_str ());
      break;

    case CMD_POST:
      // ignored.
      break;

    case CMD_PROTOVER:
      // Send xboard the list of features we support.
      fprintf (out, "feature done=0\n");
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
      fprintf (out, "feature myname=\"%s v. %s\"\n", ENGINE_ID_STR, VERSION);
      fprintf (out, "feature colors=0\n");
      fprintf (out, "feature ics=1\n");
      fprintf (out, "feature name=1\n");
      fprintf (out, "feature pause=1\n");
      fprintf (out, "feature done=1\n");
      break;

    case CMD_QMARK:
      // ignored.
      break;

    case CMD_RANDOM:
      // ignored.
      break;

    case CMD_RATING:
      // ignored.
      break;

    case CMD_REJECTED:
      // ignored.
      break;

    case CMD_REMOVE:
      // ignored.
      break;

    case CMD_RESULT:
      // ignored.
      break;

    case CMD_RESUME:
      // ignored.
      break;

    case CMD_VARIANT:
      // ignored.
      break;

    case CMD_XBOARD:
      // Set Xboard mode.
      xboard = true;
      return set_xboard_mode (tokens);
      break;

    default:
      cout << token << endl;
      assert (0);
      break;
    }

  return true;
}

//////////////
// Commands //
//////////////

// Set up time controls from level command.
bool
Session::level (const string_vector &ctokens) {
  if (ctokens.size () < 3)
    return true;

  int moves_per_control = 0, time_per_control = 0, increment = 0;
  string_vector tokens = ctokens;
  string field = first (tokens);

  // Field 1: Parse the moves per time control.
  if (!is_number (field)) assert (0);
  moves_per_control = to_int (field);
  tokens = rest (tokens);

  // Field 2: Parse the number of seconds per time control. This
  // may be written as "<minutes>:<second>".
  field = first (tokens);
  size_t idx = field.find (':');
  int minutes = 0, seconds = 0;
  if (idx != string::npos)
    {
      if (idx != 0) minutes = to_int (field.substr (0, idx));
      seconds = to_int (field.substr (idx + 1));
    }
  else
    {
      minutes = to_int (field);
    }
  time_per_control = 1000 * (60 * minutes + seconds);
  tokens = rest (tokens);

  // Field 3: Parse the incremental time bonus.
  field = first (tokens);
  increment = to_int (field);

  // Set engine time controls.
  se.set_level (moves_per_control, time_per_control, increment);

  return true;
}

bool
Session::display_time_controls (const string_vector &ctokens IS_UNUSED) {
  fprintf (out, "mode:            ");

  switch (se.controls.mode)
    {
    case UNLIMITED:    fprintf (out, "UNLIMITED"); break;
    case CONVENTIONAL: fprintf (out, "CONVENTIONAL"); break;
    case ICS:          fprintf (out, "ICS"); break;
    case EXACT:        fprintf (out, "EXACT"); break;
    }

  fprintf (out, "\n");
  fprintf (out, "moves_ptc:       %i\n", se.controls.moves_ptc);
  fprintf (out, "time_ptc:        %i\n", se.controls.time_ptc);
  fprintf (out, "increment:       %i\n", se.controls.increment);
  fprintf (out, "fixed_time:      %i\n", se.controls.fixed_time);
  fprintf (out, "fixed_depth:     %i\n", se.controls.fixed_depth);
  fprintf (out, "time_remaining:  %i\n", se.controls.time_remaining);
  fprintf (out, "moves_remaining: %i\n", se.controls.moves_remaining);

  return true;
}

bool
Session::display_help (const string_vector &ctokens IS_UNUSED) {
  // Write out help for user commands.
  for (int i = 0; i < CMD_COUNT; i++)
    {
      cout << setfill ('.');
      if (commands[i].kind == USER_CMD)
        {
          cout << left << setw (15) << commands[i].str;
          cout << left << setw (10) << commands[i].doc;
          cout << endl;
        }
    }
  cout << setfill (' ');
  return true;
}
