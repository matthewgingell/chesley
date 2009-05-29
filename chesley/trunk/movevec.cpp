////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// movevec.cpp                                                                //
//                                                                            //
// Operations on a list of moves.                                             //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <cstring>
#include <iostream>

#include "board.hpp"

using namespace std;

// Constructor for move vectors.
Move_Vector::Move_Vector () {
  count = 0;
}

Move_Vector::Move_Vector (const Move &m) {
  move[0] = m;
  count = 1;
}

Move_Vector::Move_Vector (const Move_Vector &mv) {
  memcpy (move, mv.move, sizeof (Move) * mv.count);
  count = mv.count;
}

Move_Vector::Move_Vector (const Move &m, const Move_Vector &mv) {
  move[0] = m;

  if (mv.count > 0)
    {
      memcpy (move + 1, mv.move, sizeof (Move) * mv.count);
      count = mv.count + 1;
    }
  else
    {
      count = 1;
    }
}

Move_Vector::Move_Vector (const Move_Vector &mv, const Move &m) {
  memcpy (move, mv.move, sizeof (Move) * mv.count);
  move[count] = m;
  count = mv.count + 1;
}

Move_Vector::Move_Vector (const Move_Vector &mvl, const Move_Vector &mvr) {
  memcpy (move, mvl.move, sizeof (Move) * mvl.count);
  memcpy (move + mvl.count, mvr.move, sizeof (Move) * mvr.count);
  count = mvl.count + mvr.count;
}

Move_Vector::Move_Vector (const Board &b) {
  count = 0;
  b.gen_moves (*this);
}

// Print a move vector
ostream &
operator<< (ostream &os, const Move_Vector &moves) {
  for (int i = 0; i < moves.count; i++)
    os << moves.move[i] << endl;
  return os;
}

