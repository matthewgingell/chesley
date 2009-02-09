/*
  Various utility functions.

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

#define IS_UNUSED __attribute__ ((unused))

/********************************************/
/* Various miscellaneous utility functions. */
/********************************************/

// Return a string of spaces.
static std::string spaces (int n) IS_UNUSED;

// Downcase a string.
static std::string downcase (std::string &s) IS_UNUSED;

// Test whether is string is a number.
static bool is_number (const std::string &s) IS_UNUSED;

// Convert a string to a long.
static int to_int (const std::string &s) IS_UNUSED;

// Return a malloc'd copy of a char *.
static char *newstr (const char *s) IS_UNUSED;

// Test whether a file descriptor has IO waiting.
static bool fdready (int fd) IS_UNUSED;

// Return the amount of user time we have consumed in microseconds. 
static double user_time () IS_UNUSED;

// Quick sort implementation.
template <typename T> inline void quick_sort (T &items) IS_UNUSED;

// Bubble sort implementation.
template <typename T> inline void bubble_sort (T &items) IS_UNUSED;

// Collect space seperated tokens in a vector.
typedef std::vector <std::string> string_vector;
static string_vector tokenize (const std::string &s) IS_UNUSED;

/***************************************/
/* Inline implementation of utilities. */
/***************************************/

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

// Convert a string to a long.
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

// Return the amount of user time we have consumed in microseconds. 
static double 
user_time () {
  struct rusage ru;

  getrusage (RUSAGE_SELF, &ru);
  return 
    ((double) ru.ru_utime.tv_sec)  + 
    ((double) ru.ru_utime.tv_usec) / (1000L * 1000L);
}


// Inline quicksort. Client type must 1) define a function count, 2)
// define a function value and 3) be accessible with operator[].
template <typename T> 
inline int 
partition (T &items, int left, int right, int pivot_index) {
  int pivot_value = value (items[pivot_index]);
  int store_index = left;

  std::swap (items[pivot_index], items[right]);

  for (int i = left; i <= right - 1; i++)
    {
      if (value (items[i]) < pivot_value)
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
  quick_sort_in_place (items, 0, count (items));
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

#endif // __UTIL__
