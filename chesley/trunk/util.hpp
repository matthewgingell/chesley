/*
  Various small utility functions.

  Matthew Gingell
  gingell@adacore.com
*/

#ifndef __UTIL__
#define __UTIL__

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <sys/resource.h>
#include <sys/select.h>

/* Utility macros. */

#if 0
#define UASSERT(e)  \
    ((void) ((e) ? 0 : __assert (#e, __FILE__, __LINE__)))
#define __ASSERT(e, file, line) \
  ((void)printf ("%s:%u: failed assertion `%s'\n", file, line, e), abort())
#endif

#define IS_UNUSED __attribute__ ((unused))

/********************/
/* String functions */
/********************/

// Return a string of spaces.
static std::string spaces (int n) IS_UNUSED;

// Downcase a string.
static std::string downcase (std::string &s) IS_UNUSED;

// Test whether is string is a number.
static bool is_number (const std::string &s) IS_UNUSED;

// Convert a string to an integer.
static int to_int (const std::string &s) IS_UNUSED;

// Return a malloc'd copy of a char *.
static char *newstr (const char *s) IS_UNUSED;

/**************************/
/* String vector functions */
/**************************/

typedef std::vector <std::string> string_vector;

// Collect space seperated tokens in a vector.
static string_vector tokenize (const std::string &s) IS_UNUSED;

// Return the first element of a string_vector.
static std::string first (const string_vector &in) IS_UNUSED;

// Return all but the first element of a string_vector.
static string_vector rest (const string_vector &in) IS_UNUSED;

// Return a slice of the string_vector, from 'from' to 'to' inclusive.
static string_vector slice 
(const string_vector &in, int first, int last) IS_UNUSED;

/***********************/
/* Character functions */
/***********************/

// As atoi (char *).
static long atoi (char) IS_UNUSED;

/****************/
/* IO functions */
/****************/

// Test whether a file descriptor has IO waiting.
static bool fdready (int fd) IS_UNUSED;

// Get a line, remove the trailing new line if any, and return a
// malloced string.
static char *get_line (FILE *in) IS_UNUSED;

/********************/
/* Time and timers. */
/********************/

// Return the amount of user time we have consumed in microseconds. 
static double user_time () IS_UNUSED;

/**************************/
/* Generic sorting inline */
/**************************/

// Quick sort implementation.
template <typename T> inline void 
quick_sort (T &items) IS_UNUSED;

// Bubble sort implementation.
template <typename T> inline void 
bubble_sort (T &items) IS_UNUSED;

// Insertion sort implementation. 
template <typename T> inline void 
insertion_sort (T &items) IS_UNUSED;

/******************/
/* Random numbers */
/*****************/

// Seed the random number generator
static void seed_random () IS_UNUSED;;

/********************/
/* String functions */
/********************/

// Return a string of N spaces.
static std::string 
spaces (int n) {
  return std::string (n, ' ');
}

// Downcase a string of spaces.
static std::string 
downcase (std::string &s) {
  for (uint32 i = 0; i < s.length (); i++)
    {
      s[i] = tolower (s[i]);
    }

  return s;
}

// Test whether is string is a number.
static bool 
is_number (const std::string &s) {
  for (uint32 i = 0; i < s.length (); i++)
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

// Copy a string.
static char *
newstr (const char *s) {
  int sz = strlen (s) + 1;
  char *rv = (char *) malloc (sz + 1);
  memcpy (rv, s, sz + 1);
  return rv;
}

// As atoi (char *).
static long atoi (char c) {
  assert (isdigit (c));
  return (long) c - (long) '0';
}

/**************************/
/* String vector functions */
/**************************/

// Collect space seperated tokens in a vector.
typedef std::vector <std::string> string_vector;

static string_vector
tokenize (const std::string &s) {
  uint32 first, last;
  string_vector tokens;

  first = last = 0;
  while (1)
    {
      // Find begining of token.
      for (first = last; 
	   isspace (s[first]) && first < s.length(); 
	   first++);

      // Find end of token.
      for (last = first; 
	   !isspace (s[last]) && last < s.length(); 
	   last++);

      // Collect token.
      if (first != last) 
	{ 
	  tokens.push_back (s.substr (first, last - first));
	}

      // Break at end of input.
      if (last == s.length()) 
	{
	  break;
	}
    }

  return tokens;
}

// Return the first element of a string_vector.
static std::string first (const string_vector &in) {
  return in[0];
}

// Return all but the first element of a string_vector.
static string_vector rest (const string_vector &in) {
  return slice (in, 1, in.size () - 1);
}

// Return a slice of the string_vector, from 'from' to 'to' inclusive.
static string_vector 
slice (const string_vector &in, int first, int last) {
  string_vector out;
  for (int i = first; i <= last; i++) out.push_back (in[i]);
  return out;
}

/****************/
/* IO functions */
/****************/

// Check a file descriptor and return true is there is data available
// to read from it.
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

// Get a line, remove the trailing new line if any, and return a
// malloc'd string.
static char *get_line (FILE *in) {
  const int BUFSIZE = 4096;
  char buf[BUFSIZE];

  if (!fgets (buf, BUFSIZE, in))
    {
      return NULL;
    }
  else
    {
      int last = strlen (buf) - 1;
      if (buf[last] == '\n')
	{
	  buf[last] = '\0';
	}
    }

  return newstr (buf);
}

/********************/
/* Time and timers. */
/********************/

// Return the amount of user time we have consumed in microseconds. 
static double 
user_time () {
  struct rusage ru;

  getrusage (RUSAGE_SELF, &ru);
  return 
    ((double) ru.ru_utime.tv_sec)  + 
    ((double) ru.ru_utime.tv_usec) / (1000L * 1000L);
}

/**************************/
/* Generic sorting inline */
/**************************/

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

// Good old bubble sort! Client type must 1) define a function count, 2)
// define a function value and 3) be accessible with operator[].
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

// Insertion sort implementation. 
template <typename V, typename I> 
inline void 
insertion_sort (V &items) {
  int len = count (items);
  for (int i = 1; i < len; i++)
    {
      I index = items[i];
      int j = i;

      while ((j > 0) && (value (items[j - 1]) > value (index)))
	{
	  items[j] = items[j - 1];
	  j = j - 1;
	}
      items[j] = index;
    }
}

/******************/
/* Random numbers */
/*****************/

// Seed the random number generator
static void 
seed_random () {
  srandomdev();
}

#endif // __UTIL__
