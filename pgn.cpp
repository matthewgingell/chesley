///////////////////////////////////////////////////////////////////////////////
//                                                                            //
// pgn.hpp                                                                    //
//                                                                            //
// Utilities for working with Portable Game Notation files.                   //
//                                                                            //
// Copyright Matthew Gingell <matthewgingell@gmail.com>, 2009-2015. Chesley   //
// the Chess Engine! is free software distributed under the terms of the      //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include "pgn.hpp"
#include "util.hpp"

// debug only
#include <iostream>

using namespace std;

/////////////////////////////
// Operations on PGN files //
/////////////////////////////

// Initialize the stream.
PGN :: PGN () {
  fp = NULL;
  status = OK;
}

// Open a stream from a file.
void
PGN :: open (const char *filename) {
  fp = fopen (filename, "r");
  if (!fp) status = FATAL_ERROR;
}

// Close the stream.
void
PGN :: close () {
  fclose (fp);
  fp = NULL;
}

// Read a game from the file.
Game
PGN::read_game () {
  Game g;

  try
    {
      if (status == RECOVERABLE_ERROR)
        {
          char c;
          while (true)
            {
              c = fgetc (fp);
              if (c == '[')
                {
                  status = OK;
                  ungetc (c, fp);
                  break;
                }
              if (c == EOF)
                {
                  status = END_OF_FILE;
                  break;
                }
            }
        }


      skip_comment_and_whitespace ();

      if (feof (fp))
        {
          status = END_OF_FILE;
          return g;
        }

      read_metadata (g);
      skip_comment_and_whitespace ();

      if (feof (fp))
        {
          status = FATAL_ERROR;
          return g;
        }

      read_moves (g);
    }
  catch (string s)
    {
      cerr << "Parse error reading PGN:" + s << endl;
      status = RECOVERABLE_ERROR;
    }

  return g;
}

// Skip a '{ ... }' comment or white space.
void
PGN::skip_comment_and_whitespace () {
  char c;
  while (true)
    {
      c = fgetc (fp);

      // Skip a comment.
      if (c == '{')
        {
          while (true)
            {
              c = fgetc (fp);
              if (c == EOF || c == '}')
                {
                  break;
                }
            }
        }

      // Skip white space.
      else if (isspace (c))
        {
          continue;
        }

      // Otherwise break out of the loop.
      else
        {
          ungetc (c, fp);
          break;
        }
    }
}

// Skip a '( ... )' in the moves list.
void
PGN::skip_recursive_variation () {
  int depth = 0;
  while (true)
    {
      char c = fgetc (fp);

      if (c == '(')
        {
          depth++;
        }
      else if (c == ')')
        {
          depth--;
        }

      if (depth == 0)
        {
          break;
        }
    }
}

// Read the meta data at the current offset.
void
PGN::read_metadata (Game &g) {
  string key;
  string value;

  while (true)
    {
      key.clear ();
      value.clear ();
      skip_comment_and_whitespace ();
      char c = fgetc (fp);

      // Read the opening square bracket.
      if (c != '[')
        {
          ungetc (c, fp);
          return;
        }

      // Read the key.
      while (true)
        {
          c = fgetc (fp);
          if (isspace (c))
            {
              ungetc (c, fp);
              break;
            }
          else
            {
              key.push_back (c);
            }
        }

      // Read the value.
      while (true)
        {
          c = fgetc (fp);
          if (c == EOF)
            {
              status = FATAL_ERROR;
              return;
            }
          else if (c == '"')
            {
              while ((c = fgetc (fp)) != '"')
                {
                  value.push_back (c);
                }
            }
          else if (c == ']')
            {
              break;
            }
          else
            {
              value.push_back (c);
            }
        }

      // Store the key/value pair.
      key = trim (key);
      value = trim (value);
      g.metadata[key] = value;
    }
}

// Read a list of moves from the steam.
void
PGN::read_moves (Game &g) {
  char c;
  string move_number;
  string san;
  string eog;

  // Set up the internal board.
  if (g.metadata.find ("FEN") != g.metadata.end())
    {
      b = Board::from_fen (g.metadata["FEN"]);
    }
  else
    {
      b = Board::startpos ();
    }

  skip_comment_and_whitespace ();
  while (true)
    {
      skip_comment_and_whitespace ();
      san.clear ();
      c = fgetc (fp);

      // Return on EOF.
      if (c == EOF)
        {
          return;
        }

      // Recognize a recursive variation.
      else if (c == '(')
        {
          ungetc (c, fp);
          skip_recursive_variation ();
        }

      // Recognize the "*" game terminator.
      else if (c == '*')
        {
          eog = "*";
          goto EOL;
        }

      // Recognize a move number or a game terminator.
      else if (isdigit (c))
        {
          char next = fgetc (fp);

          if (next == '/' || next == '-')
            {
              eog.push_back (c);
              eog.push_back (next);
              while (true)
                {
                  c = fgetc (fp);
                  if (!isspace (c))
                    {
                      eog.push_back (c);
                    }
                  else
                    {
                      break;
                    }
                }
              goto EOL;
            }
          else
            {
              ungetc (next, fp);
              while (true)
                {
                  c = fgetc (fp);
                  if (c == EOF || isdigit (c) || c == '.')
                    {
                      continue;
                    }
                  else
                    {
                      ungetc (c, fp);
                      break;
                    }
                }
            }
        }

      // Read a move in SAN format.
      else
        {
          ungetc (c, fp);

          while (true)
            {
              c = fgetc (fp);

              if (isspace (c) || c == EOF)
                {
                  break;
                }
              else
                {
                  san.push_back (c);
                }
            }
        }

      // Skip ignored tokens.
      if (san == "="   ||
          san == "!"   ||
          san == "?"   ||
          san == "+="  || san == "=+"  ||
          san == "+/-" || san == "-/+" ||
          san == "+-"  || san == "-+")
        {
          continue;
        }

      if (san.length () > 0)
        {
          Move m = b.from_san (san);
          if (!b.apply (m))
            {
              throw string ("Got bad move: " + san);
            }
          else
            {
              g.moves.push_back (m);
            }
        }
    }

 EOL:

  // Set the game outcome.
  if (eog == "1-0")
    {
      g.winner = WHITE;
    }
  else if (eog == "0-1")
    {
      g.winner = BLACK;
    }
  else if (eog == "1/2-1/2")
    {
      g.winner = NULL_COLOR;
    }
  else
    {
      throw string ("Bad result string: " + eog);
    }

  return;
}
