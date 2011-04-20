////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// util.hpp                                                                   //
//                                                                            //
// Various small utility functions. This file is intended to encapsulate      //
// all the platform dependent functionality used by Chesley.                  //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2011. Chesley             //
// the Chess Engine! is free software distributed under the terms of the      //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef __UTIL__
#define __UTIL__

#include <cassert>
#include <cstdio>
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

///////////////////////
// Macro definitions //
///////////////////////

#ifdef __GNUC__
#define IS_CONST    __attribute__ ((const))
#define IS_UNUSED   __attribute__ ((unused))
#define ALIGNED(A)  __attribute__ ((aligned (A)))
#else
#define IS_CONST
#define IS_UNUSED
#define ALIGNED(A)
#endif // __GNUC_

#define ZERO(object) memset (object, 0, sizeof (object));

static int32 sign (int x) IS_UNUSED;
static int32 sign (int x) { return x / abs (x); }

//////////////////////
// String functions //
//////////////////////

// Return a string of spaces.
std::string spaces (int n);

// Downcase a string.
std::string downcase (std::string &s);

// Upcase a string.
std::string upcase (std::string &s);

// Test whether is string is a number.
bool is_number (const std::string &s);

// Convert a string or character to an integer.
int to_int (const std::string &s);
int to_int (const char &c);

// Return a malloc'd copy of a char *.
char *newstr (const char *s);

// Trim leading and trailing white space.
std::string trim (const std::string &s);

////////////////////////////
// String vector functions //
////////////////////////////

typedef std::vector <std::string> string_vector;

// Collect space separated tokens in a vector.
string_vector tokenize (const std::string &s);

// Return a slice of the string_vector, from 'from' to 'to' inclusive.
string_vector slice
(const string_vector &in, size_t first, size_t last);

// Return a slice of the string_vector, from 'from' then end.
string_vector slice
(const string_vector &in, size_t first);

// Return the first element of a string_vector.
std::string first (const string_vector &in);

// Return all but the first element of a string_vector.
string_vector rest (const string_vector &in);

// Return a string build from joining together each element of in. If
// delim is not zero, it is used as the field separator.
std::string
join (const string_vector &in, const std::string &delim);

// Stream insertion.
std::ostream &
operator<< (std::ostream &os, const string_vector &in);

/////////////////////////
// Character functions //
/////////////////////////

// As atoi (char /).
long atoi (char);

//////////////////
// IO functions //
//////////////////

// Test whether a file descriptor has IO waiting.
bool fdready (int fd);

// Get a line, remove the trailing new line if any, and return a
// malloc'd string.
char *get_line (FILE *in);

// Advance over white space characters and return the number skipped.
int skip_whitespace (FILE *in);

/////////////////////
// Time and timers //
/////////////////////

// Return the time in milliseconds since the epoch.
uint64 mclock ();

// Return the amount of CPU time used in milliseconds.
uint64 cpu_time ();

#ifdef _WIN32
#define usleep(usecs) (Sleep (usecs / 1000))
#endif // _WIN32

////////////////////////////
// Generic sorting inline //
////////////////////////////

// Quick sort implementation.
template <typename T> inline void
quick_sort (T &items);

// Bubble sort implementation.
template <typename T> inline void
bubble_sort (T &items);

// Insertion sort implementation.
template <typename T> inline void
insertion_sort (T &items);;

////////////////////
// Random numbers //
///////////////////

// Seed the random number generator
void seed_random ();

// Return a 64-bit random number.
uint64 random64 ();

#endif // __UTIL__
