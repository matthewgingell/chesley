////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// util.hpp                                                                   //
//                                                                            //
// Various small utility functions. This file is indended to encapsulate      //
// all the platform dependant functionality used by Chesley.                  //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef __UTIL__
#define __UTIL__

#include <assert.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#ifndef _WIN32
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/select.h>
#else
#include <windows.h>
#endif // _WIN32

#include "common.hpp"

extern char *arg0;

////////////////////////
// Macro definitions. //
////////////////////////

#ifdef __GNUC__
#define IS_CONST __attribute__ ((const))
#define IS_UNUSED  __attribute__ ((unused))
#define ALIGNED(A)  __attribute__ ((aligned (A)))
#else
#define IS_CONST
#define IS_UNUSED
#define ALIGHT(A)
#endif // __GNUC_

static int32 sign (int x) IS_UNUSED;
static int32 sign (int x) { return x / abs (x); }

//////////////////////
// String functions //
//////////////////////

// Return a string of spaces.
static std::string spaces (int n) IS_UNUSED;

// Downcase a string.
static std::string downcase (std::string &s) IS_UNUSED;

// Upcase a string.
static std::string upcase (std::string &s) IS_UNUSED;

// Test whether is string is a number.
static bool is_number (const std::string &s) IS_UNUSED;

// Convert a string or character to an integer.
static int to_int (const std::string &s) IS_UNUSED;
static int to_int (const char &c) IS_UNUSED;

// Return a malloc'd copy of a char *.
static char *newstr (const char *s) IS_UNUSED;

// Trim leading and trailing whitespace.
static std::string trim (const std::string &s) IS_UNUSED;

////////////////////////////
// String vector functions //
////////////////////////////

typedef std::vector <std::string> string_vector;

// Collect space separated tokens in a vector.
static string_vector tokenize (const std::string &s) IS_UNUSED;

// Return a slice of the string_vector, from 'from' to 'to' inclusive.
static string_vector slice
(const string_vector &in, size_t first, size_t last) IS_UNUSED;

// Return a slice of the string_vector, from 'from' then end.
static string_vector slice
(const string_vector &in, size_t first) IS_UNUSED;

// Return the first element of a string_vector.
static std::string first (const string_vector &in) IS_UNUSED;

// Return all but the first element of a string_vector.
static string_vector rest (const string_vector &in) IS_UNUSED;

// Return a string build from joining together each element of in. If
// delim is not zero, it is used as the field separator.
static std::string
join (const string_vector &in, const std::string &delim) IS_UNUSED;

// Stream insertion.
static std::ostream &
operator<< (std::ostream &os, const string_vector &in) IS_UNUSED;

/////////////////////////
// Character functions //
/////////////////////////

// As atoi (char /).
static long atoi (char) IS_UNUSED;

//////////////////
// IO functions //
//////////////////

// Test whether a file descriptor has IO waiting.
static bool fdready (int fd) IS_UNUSED;

// Get a line, remove the trailing new line if any, and return a
// malloc'd string.
static char *get_line (FILE *in) IS_UNUSED;

// Advance over whitespace characters and return the number skipped.
static int skip_whitespace (FILE *in) IS_UNUSED;

//////////////////////
// Time and timers. //
//////////////////////

// Return the time in milliseconds since the epoch.
static uint64 mclock () IS_UNUSED;

// Return the amount of CPU time used in milliseconds.
static uint64 cpu_time () IS_UNUSED;

////////////////////////////
// Generic sorting inline //
////////////////////////////

// Quick sort implementation.
template <typename T> inline void
quick_sort (T &items) IS_UNUSED;

// Bubble sort implementation.
template <typename T> inline void
bubble_sort (T &items) IS_UNUSED;

// Insertion sort implementation.
template <typename T> inline void
insertion_sort (T &items) IS_UNUSED;

////////////////////
// Random numbers //
///////////////////

// Seed the random number generator
void seed_random ();

// Return a 64-bit random number.
uint64 random64 ();

//////////////////////
// String functions //
//////////////////////

// Return a string of N spaces.
static std::string
spaces (int n) {
  return std::string (n, ' ');
}

// Down case a string of spaces.
static std::string
downcase (std::string &s) {
  for (size_t i = 0; i < s.length (); i++)
    {
      s[i] = tolower (s[i]);
    }

  return s;
}

// Upcase a string.
static std::string upcase (std::string &s) {
  for (size_t i = 0; i < s.length (); i++)
    {
      s[i] = toupper (s[i]);
    }
  return s;
}

// Test whether is string is a number.
static bool
is_number (const std::string &s) {
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
static int
to_int (const std::string &s) {
  return atoi (s.c_str ());
}

// Convert a character to an integer.
static int
to_int (const char &c) {
  return c - '0';
}

// Copy a string.
static char *
newstr (const char *s) {
  size_t sz = strlen (s) + 1;
  char *rv = (char *) malloc (sz + 1);
  memcpy (rv, s, sz + 1);
  return rv;
}

// Trim leading and trailing whitespace.
static std::string 
trim (const std::string &s) {
  std::string rv = s;
  size_t first = rv.find_first_not_of (" \n\t\r");
  size_t last = rv.find_last_not_of (" \n\t\r");
  if (first != std::string::npos) rv.erase (0, first);
  if (last != std::string::npos && last < rv.length () - 1) 
    rv.erase (last + 1);
  return rv;
}

// As atoi (char *).
static long atoi (char c) {
  assert (isdigit (c));
  return (long) c - (long) '0';
}

////////////////////////////
// String vector functions //
////////////////////////////

// Collect space or ';' separated tokens in a vector. Quoted fields
// are not broken.
typedef std::vector <std::string> string_vector;

static string_vector
tokenize (const std::string &s) {
  string_vector tokens;
  std::string token;
  bool in_quote = false;

  for (std::string::const_iterator i = s.begin (); i < s.end (); i++)
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
static string_vector
slice (const string_vector &in, size_t first, size_t last) {
  string_vector out;
  for (size_t i = first; i <= last; i++) out.push_back (in[i]);
  return out;
}

// Return a slice of the string_vector, from 'from' then end.
static string_vector
slice (const string_vector &in, size_t first) {
  return slice (in, first, in.size () - 1);
}

// Return the first element of a string_vector.
static std::string first (const string_vector &in) {
  return in[0];
}

// Return all but the first element of a string_vector.
static string_vector rest (const string_vector &in) {
  return slice (in, 1);
}

// Return a string build from joining together each element of in. If
// delim is not zero, it is used as the field separator.
static std::string join
(const string_vector &in, const std::string &delim) {
  string_vector ::const_iterator i;
  std::string out = "";

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
static std::ostream &
operator<< (std::ostream &os, const string_vector &in) {
  return os << join (in, ", ");
}

////////////////////
// I/O functions. //
////////////////////

// Check a file descriptor and return true is there is data available
// to read from it.
#ifndef _WIN32
static bool
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
#else
static bool
fdready (int fd IS_UNUSED) {
  return 0;
}
#endif  // _WIN32

// Get a line, remove the trailing new line if any, and return a
// malloc'd string.
static char *get_line (FILE *in) {
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

// Advance over whitespace characters and return the number skipped.
static int 
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

//////////////////////
// Time and timers. //
//////////////////////

// Return the time in milliseconds since the epoch.
static uint64
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
static uint64
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

// Inline quicksort. Client type must 1) define a function count, 2)
// define a function value and 3) be accessible with operator[].
template <typename T>
inline int
partition (T &items, int left, int right, int pivot_index) {
  int pivot_value = value (items[pivot_index]);
  int store_index = left;

  assert (left >= 0);
  assert (right < count (items));

  std::swap (items[pivot_index], items[right]);

  for (int i = left; i < right; i++)
    {
      if (value (items[i]) <= pivot_value)
        {
          std::swap (items [i], items[store_index]);
          store_index++;
        }
    }

  std::swap (items[store_index], items[right]);
  return store_index;
}

template <typename T>
inline void
quick_sort_in_place (T &items, int left, int right) {
  if (right > left)
    {
      int pivot_index = left;
      int new_pivot_index = partition (items, left, right, pivot_index);
      quick_sort_in_place (items, left, new_pivot_index - 1);
      quick_sort_in_place (items, new_pivot_index + 1, right);
    }
}

template <typename T>
inline void
quick_sort (T &items) {
  quick_sort_in_place (items, 0, count (items) - 1);
}

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
              std::swap (items[i], items[i + 1]);
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

/////////////////////////////
// Miscellaneous routines. //
/////////////////////////////

template <typename elt>
inline void swap (elt &l, elt &r) {
  elt tmp;
  memcpy (&tmp, &l, sizeof (elt));
  memcpy (&l,   &r, sizeof (elt));
  memcpy (&r, &tmp, sizeof (elt));
}

#endif // __UTIL__
