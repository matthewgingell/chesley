////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// util.cpp                                                                   //
//                                                                            //
// Various small utility functions. This file is intended to encapsulate      //
// all the platform dependent functionality used by Chesley.                  //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2011. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include "util.hpp"

#include <iostream>

using namespace std;

//////////////////////
// String functions //
//////////////////////

// Return a string of N spaces.
string
spaces (int n) {
  return string (n, ' ');
}

// Down case a string of spaces.
string
downcase (string &s) {
  for (size_t i = 0; i < s.length (); i++)
    {
      s[i] = tolower (s[i]);
    }

  return s;
}

// Upcase a string.
string upcase (string &s) {
  for (size_t i = 0; i < s.length (); i++)
    {
      s[i] = toupper (s[i]);
    }
  return s;
}

// Test whether is string is a number.
bool
is_number (const string &s) {
  for (size_t i = 0; i < s.length (); i++)
    {
      if (!isdigit (s[i]))
        {
          return false;
        }
    }
  return true;
}

// Convert a string to an integer.
int
to_int (const string &s) {
  return atoi (s.c_str ());
}

// Convert a character to an integer.
int
to_int (const char &c) {
  return c - '0';
}

// Copy a string.
char *
newstr (const char *s) {
  size_t sz = strlen (s) + 1;
  char *rv = (char *) malloc (sz);
  memcpy (rv, s, sz);
  return rv;
}

// Trim leading and trailing white space.
string 
trim (const string &s) {
  string rv = s;
  size_t first = rv.find_first_not_of (" \n\t\r");
  size_t last = rv.find_last_not_of (" \n\t\r");
  if (first != string::npos) rv.erase (0, first);
  if (last != string::npos && last < rv.length () - 1) 
    rv.erase (last + 1);
  return rv;
}

// As atoi (char *).
long atoi (char c) {
  assert (isdigit (c));
  return (long) c - (long) '0';
}

////////////////////////////
// String vector functions //
////////////////////////////

// Collect space or ';' separated tokens in a vector. Quoted fields
// are not broken.
typedef vector <string> string_vector;

string_vector
tokenize (const string &s) {
  string_vector tokens;
  string token;
  bool in_quote = false;

  for (string::const_iterator i = s.begin (); i < s.end (); i++)
    {
      if (*i == '\"')
        {
          in_quote = !in_quote;
        }

      if (!in_quote
          && (isspace (*i) || *i == ';'))
        {
          if (token.length ())
            {
              tokens.push_back (token);
              token.clear ();
            }
        }
      else
        {
          if (*i != '\"')
            {
              token += *i;
            }
        }
    }

  if (token.length ())
    {
      tokens.push_back (token);
    }

  return tokens;
}

// Return a slice of the string_vector, from 'from' to 'to' inclusive.
string_vector
slice (const string_vector &in, size_t first, size_t last) {
  string_vector out;
  for (size_t i = first; i <= last; i++) out.push_back (in[i]);
  return out;
}

// Return a slice of the string_vector, from 'from' then end.
string_vector
slice (const string_vector &in, size_t first) {
  return slice (in, first, in.size () - 1);
}

// Return the first element of a string_vector.
string first (const string_vector &in) {
  return in[0];
}

// Return all but the first element of a string_vector.
string_vector rest (const string_vector &in) {
  return slice (in, 1);
}

// Return a string build from joining together each element of in. If
// delim is not zero, it is used as the field separator.
string join
(const string_vector &in, const string &delim) {
  string_vector ::const_iterator i;
  string out = "";

  for (i = in.begin (); i < in.end (); i++)
    {
      out += *i;
      if (i + 1 < in.end () && delim.length () > 0)
        {
          out += delim;
        }
    }

  return out;
}

// Stream insertion.
ostream &
operator<< (ostream &os, const string_vector &in) {
  return os << join (in, ", ");
}

///////////////////
// I/O functions //
///////////////////

extern bool xboard;

// Check a file descriptor and return true is there is data available
// to read from it.
#ifndef _WIN32
bool
fdready (int fd) {
  fd_set readfds;
  struct timeval timeout;

  // Initialize the set of descriptors to test.
  FD_ZERO (&readfds);
  FD_SET (fd, &readfds);

  // Initialize timeout.
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;

  // Poll the file descriptor.
  return select (fd + 1, &readfds, NULL, NULL, &timeout);
}
#else // _WIN32

#include <conio.h>

// Asynchronous IO code for Windows taken directly from Olithink,
// Oliver Brausch 2008.
bool
fdready (int fd IS_UNUSED) {
  static int      init = 0, pipe;
  static HANDLE   inh;
  DWORD           dw;

  if (xboard) // Xboard sends input commands over the internal pipe
    {
      if (!init)
        {
          init = 1;
          inh = GetStdHandle(STD_INPUT_HANDLE);
          pipe = !GetConsoleMode(inh, &dw);
          if (!pipe)
            {
              SetConsoleMode(inh,
                             dw & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT));
              FlushConsoleInputBuffer(inh);
            }
        }
      if (pipe)
        {
          if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL))
            return 1;
          return dw;
        }
      else
        {
          GetNumberOfConsoleInputEvents(inh, &dw);
          return dw <= 1 ? 0 : dw;
        }
    }
  else // not XBoard, so use _kbhit() for getting user input
    {
      return _kbhit();
    }
}
#endif  // _WIN32

// Get a line, remove the trailing new line if any, and return a
// malloc'd string.
char *get_line (FILE *in) {
  const int BUFSIZE = 4096;
  static char buf[BUFSIZE];

  if (!fgets (buf, BUFSIZE, in))
    {
      return NULL;
    }
  else
    {
      size_t last = strlen (buf) - 1;
      if (buf[last] == '\n')
        {
          buf[last] = '\0';
        }
    }

  return newstr (buf);
}

// Advance over white space characters and return the number skipped.
int 
skip_whitespace (FILE *in) {
  int count = 0;
  char c;  
  while ((c = fgetc (in)))
    {
      if (c == EOF) 
        {
          break;
        }
      else if (!isspace (c)) 
        {
          ungetc (c, in);          
          break;
        }
      count++;
    }

  return count;
}

/////////////////////
// Time and timers //
/////////////////////

// Return the time in milliseconds since the epoch.
uint64
mclock () {
#ifndef _WIN32
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return
    ((uint64) tv.tv_sec) * 1000 +
    ((uint64) tv.tv_usec) / 1000;
#else 
  LARGE_INTEGER tick, ticks_per_second;
  QueryPerformanceFrequency (&ticks_per_second);
  QueryPerformanceCounter (&tick);
  return (tick.QuadPart * 1000) / ticks_per_second.QuadPart;
#endif // _WIN32
}

// Return the amount of CPU time used in milliseconds.
uint64
cpu_time () {
#ifndef _WIN32
  struct rusage ru;
  getrusage (RUSAGE_SELF, &ru);
  return
    ((uint64) ru.ru_utime.tv_sec) * 1000
    + ((uint64) ru.ru_utime.tv_usec) / 1000;
#else // _WIN32
  LARGE_INTEGER tick, ticks_per_second;
  QueryPerformanceFrequency (&ticks_per_second);
  QueryPerformanceCounter (&tick);
  return (tick.QuadPart * 1000) / ticks_per_second.QuadPart;
#endif  // _WIN32
}

////////////////////////////
// Generic sorting inline //
////////////////////////////

// Bubble sort. Client type must 1) define a function count, 2) define
// a function value and 3) be accessible with operator[].
template <typename T>
inline void
bubble_sort (T &items) {
  int len = count (items);
  bool done;
  do
    {
      done = true;
      for (int i = 0; i < len - 1; i++)
        {
          if (value (items[i]) > value (items[i + 1]))
            {
              swap (items[i], items[i + 1]);
              done = false;
            }
        }
      len -= 1;
    }
  while (!done);
}

// Insertion sort implementation.  Client type must 1) define a
// function count, 2) be accessible with operator[].
template <typename V, typename I, bool cmp (const I&, const I&)>
inline void
insertion_sort (V &items) {
  int len = count (items);
  for (int i = 1; i < len; i++)
    {
      I index = items[i];
      int j = i;
      while ((j > 0) && cmp(items[j - 1], index))
        {
          items[j] = items[j - 1];
          j = j - 1;
        }
      items[j] = index;
    }
}

////////////////////////////
// Miscellaneous routines //
////////////////////////////

template <typename elt>
inline void swap (elt &l, elt &r) {
  elt tmp;
  memcpy (&tmp, &l, sizeof (elt));
  memcpy (&l,   &r, sizeof (elt));
  memcpy (&r, &tmp, sizeof (elt));
}

///////////////////////////////
// Random number generation  //
///////////////////////////////

// Seed the random number generator.
void seed_random () {
  return;
}

// Return a 64-bit random number.
uint64 random64 () {
  return ((uint64) random ()) << 32 | random () ;
}
