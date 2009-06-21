////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// commands.cpp                                                               //
//                                                                            //
// Code for executing commands passed to the command interpreter.             //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <cstdio>
#include <string>

#include "chesley.hpp"

using namespace std;

///////////////////////////
//                       //
// Command declarations. //
//                       //
///////////////////////////

enum Command
  { 
    CMD_NULL = -1,

    ////////////////////
    // User commands. //
    ////////////////////

    CMD_BLACK, 
    CMD_DISP,  
    CMD_DTC, 
    CMD_EVAL, 
    CMD_FEN, 
    CMD_FORCE, 
    CMD_GO, 
    CMD_HELP,
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

    /////////////////////////
    // Debugging commands. //
    /////////////////////////

    CMD_APPLY, 
    CMD_ATTACKS, 
    CMD_BENCH, 
    CMD_DIV, 
    CMD_DUMPPAWNS, 
    CMD_EPD, 
    CMD_HASH, 
    CMD_PERFT, 
    CMD_TESTHASHING,

    //////////////////////
    // XBoard commands. //
    //////////////////////

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
  Command     code; 
  char const *str; 
  char const *usage; 
  char const *doc; 
} commands[CMD_COUNT] = {


  ////////////////////
  // User commands. //
  ////////////////////

  { CMD_BLACK,     "BLACK",     "",
    "Set user to play black." },

  { CMD_DISP,      "DISP",      "",
    "Print this position." },

  { CMD_DTC,       "DTC" ,      "",
    "Print time control settings." },

  { CMD_EVAL,      "EVAL",      "",
    "Print the static evaluation for this position."},

  { CMD_FEN,       "FEN",       "",
    "Print a FEN string for this position."},

  { CMD_FORCE,     "FORCE",     "",
    "Freeze the engine." },

  { CMD_GO,        "GO",        "",
    "Set the engine running."},

  { CMD_HELP,      "HELP",      "",
    "Print a help message."},

  { CMD_LEVEL,     "LEVEL",     "<moves time increment>",
    "Set time controls"},

  { CMD_MOVE,      "MOVE",      "<move>",                 
    "Make a move." },

  { CMD_MOVES,     "MOVES",     "",
    "Print a list of all legal moves." },

  { CMD_NEW,       "NEW",       "",
    "Start a new game." },

  { CMD_PLAYOTHER, "PLAYOTHER", "",                       
    "Swap the sides played by the engine and the user." },

  { CMD_PLAYSELF,  "PLAYSELF",  "",                       
    "Display play a computer vs. computer games."},

  { CMD_QUIT,      "QUIT",      "",
    "Quit Chesley." },

  { CMD_SD,        "SD",        "",
    "Set a fixed search depth limit."},

  { CMD_SETBOARD,  "SETBOARD",  "<fen>",                  
    "Set the board from a FEN string."},

  { CMD_ST,        "ST",        "<time>",
    "Set a fixed time per move"},

  { CMD_TIME,      "TIME",      "<centiseconds>",
    "Set time remaining on the clock."},

  { CMD_WHITE,     "WHITE",     "",
    "Set user to play black." },

  /////////////////////////
  // Debugging commands. //
  /////////////////////////

  { CMD_APPLY,     "APPLY",     "",                       
    "Apply a move." },

  { CMD_ATTACKS,   "ATTACKS",   "",                      
    "Display a map of attacked squares." },

  { CMD_BENCH,     "BENCH",     "<depth>",
    "Analyze this position to a fixed depth." },

  { CMD_DIV,       "DIV",       "<depth>",
    "Compute div to a fixed depth" },

  { CMD_DUMPPAWNS, "DUMPPAWNS", "",
    "Dump a vector of pawns."
  },

  { CMD_EPD,       "EPD",       "<epd>",
    "Evaluate an EPD string."},

  { CMD_HASH,      "HASH",      "",
    "Print the current position hash."},

  { CMD_PERFT,     "PERFT",     "<depth>",
    "Compute perft to a fixed depth."},

  { CMD_TESTHASHING, "TESTHASHING", "",
    "Run a test on hash code generation."},

  //////////////////////
  // XBoard commands. //
  //////////////////////

  { CMD_ACCEPTED,  "ACCEPTED",  "",
    ""},

  { CMD_ANALYZE,   "ANALYZE",   "",
    ""},

  { CMD_BK,        "BK",        "",
    ""},

  { CMD_COMPUTER,  "COMPUTER",  "",
    ""},

  { CMD_DRAW,      "DRAW",      "",
    ""},

  { CMD_EASY,      "EASY",      "",
    ""},

  { CMD_EDIT,      "EDIT",      "",
    ""},

  { CMD_HARD,      "HARD",      "",
    ""},

  { CMD_HINT,      "HINT",      "",
    ""},

  { CMD_ICS,       "ICS",       "",
    ""},

  { CMD_NAME,      "NAME",      "",
    ""},

  { CMD_NOPOST,    "NOPOST",    "",
    ""},

  { CMD_OTIM,      "OTIM",      "",
    "Command ignored."},
  
  { CMD_PAUSE,     "PAUSE",     "",
    ""},

  { CMD_PING,      "PING",      "",
    ""},

  { CMD_POST,      "POST",      "",
    ""},

  { CMD_PROTOVER,  "PROTOVER",  "",
    ""},

  { CMD_QMARK,     "QMARK",     "",
    ""},

  { CMD_RANDOM,    "RANDOM",    "",
    ""},

  { CMD_RATING,    "RATING",    "",
    ""},

  { CMD_REJECTED,  "REJECTED",  "",
    ""},

  { CMD_REMOVE,    "REMOVE",    "",
    ""},

  { CMD_RESULT,    "RESULT",    "",
    ""},

  { CMD_RESUME,    "RESUME",    "",
    ""},

  { CMD_USERMOVE,  "USERMOVE",      "<move>",                 
    "Make a move." },

  { CMD_VARIANT,   "VARIANT",   "",
    ""},

  { CMD_XBOARD,    "XBOARD",    "",
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

    ////////////////////
    // User commands. //
    ////////////////////

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

    case CMD_LEVEL:
      level (rest (tokens));
      break;

    case CMD_USERMOVE:
    case CMD_MOVE: 
      // Play a move.
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

    /////////////////////////
    // Debugging commands. //
    /////////////////////////

    case CMD_APPLY: 
      // Apply a move to the current position.
      running = false;
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

    case CMD_EPD: 
      // Execute an epd string.
      epd (tokens);
      break;

    case CMD_PERFT:
      // Compute perft to a fixed depth.
      {
        int depth = (to_int (tokens[1]));
        fprintf (out, "Computing perft to depth %i...\n", depth);
        uint64 start = cpu_time();
        uint64 count = board.perft (depth);
        uint64 elapsed = cpu_time() - start;
        fprintf (out, "moves = %llu\n", count);
        fprintf (out, "%.2f seconds elapsed.\n", ((double) elapsed) / 1000.0);
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
      
    //////////////////////
    // XBoard commands. //
    //////////////////////

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
      // ignored.
      break;

    case CMD_EDIT:
      // ignored.
      break;

    case CMD_HARD:
      // ignored.
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
      // fprintf (out, "feature myname=\"%s\"\n", get_prologue ());
      fprintf (out, "feature myname=\"%s\"\n", arg0);
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
      return set_xboard_mode (tokens);
      break;

    default:
      cout << token << endl;
      assert (0);
      break;
    }

  return true;
}

///////////////
// Commands. //
///////////////

// Set up time controls from level command.
bool 
Session::level (const string_vector &ctokens) {
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
          
  // Field 3: Parse to incremental time bonus.
  field = first (tokens);
  increment = to_int (field);
          
  // Set search engine time controls.
  se.set_level (moves_per_control, time_per_control, increment);

  return true;
}

bool 
Session::display_time_controls (const string_vector &ctokens IS_UNUSED) {
  fprintf (out, "mode:            ");
  switch (se.controls.mode) {
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
  for (int i = 0; i < CMD_COUNT; i++)
    {
      fprintf  (out, "%s\t%s\t%s\n", 
                commands[i].str, commands [i].usage, commands[i].doc);
    }

  return true;
}


