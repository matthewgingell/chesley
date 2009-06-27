////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// move.hpp                                                                   //
//                                                                            //
// Representation and operations on a chess move.                             //
//                                                                            //
// Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the         //
// Chess Engine! is free software distributed under the terms of the          //
// GNU Public License.                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MOVE_
#define _MOVE_

#include "types.hpp"
#include "util.hpp"

///////////////////////
//   The Move type   //
///////////////////////

struct Board;

struct Move {

  // Constructors.

  // It's important we have a null constructor here as we create large
  // arrays of moves which we never access.
  Move () {}

  // Construct a move;
  Move (coord from, coord to, Color color, Kind kind, Kind capture, 
        Kind promote = NULL_KIND, bool en_passant = false, Score score = 0) 
    : from (from), to (to), color (color), kind (kind), 
      capture (capture), promote (promote), en_passant (en_passant),
      score (score) {};
  
  // Kind of color of the piece being moved.
  inline Color get_color () const { 
    return color; 
  }

  // Get the kind of the piece being moved.
  inline Kind get_kind () const { 
    return kind; 
  }

  // Returns the piece being captured, or NULL_KIND if this is not a
  // capture.
  inline Kind get_capture () const { 
    return capture; 
  }

  // Is this move an en passant capture.
  inline bool is_en_passant () const { 
    return en_passant; 
  }

  // Is this a castling move?
  inline bool is_castle () const {
    return is_castle_qs () || is_castle_ks ();
  }

  // Is this a queen-side castle.
  inline bool is_castle_qs () const {
    return (kind == KING &&
            ((color == WHITE && from == E1 && to == C1) ||
             (color == BLACK && from == E8 && to == C8)));
  }

  // Is this a king-side castle.
  inline bool is_castle_ks () const {
    return (kind == KING &&
            ((color == WHITE && from == E1 && to == G1) ||
             (color == BLACK && from == E8 && to == G8)));
  }

  // State
  coord  from       :6;
  coord  to         :6;
  Color  color      :4;
  Kind   kind       :4;
  Kind   capture    :4;
  Kind   promote    :4;
  bool   en_passant :1;
  uint32 unused     :3;
  Score  score      :32;
};

// A type to use for null moves.
const Move NULL_MOVE (0, 0, NULL_COLOR, NULL_KIND, NULL_KIND);

// Compare moves.
inline bool
less_than (const Move &lhs, const Move &rhs) {
  return lhs.score < rhs.score;
}

// Compare moves.
inline bool
more_than (const Move &lhs, const Move &rhs) {
  return lhs.score > rhs.score;
}

// Output for moves.
std::ostream & operator<< (std::ostream &os, const Move &b);

////////////////////////////////
//    The Move vector type.   //
////////////////////////////////

struct Move_Vector;

std::ostream &
operator<< (std::ostream &os, const Move_Vector &moves);

// Vector of moves.
struct Move_Vector {

  // Amazingly, examples of positions with 218 different possible
  // moves exist.
  static uint32 const SIZE = 256;

  // Various constructors for composing Move_Vectors.
  Move_Vector ();
  Move_Vector (const Move &m);
  Move_Vector (const Move_Vector &mv);
  Move_Vector (const Move &m, const Move_Vector &mv);
  Move_Vector (const Move_Vector &mv, const Move &m);
  Move_Vector (const Move_Vector &mvl, const Move_Vector &mvr);
  Move_Vector (const Board &b);

  // Clear all data from the vector.
  void clear () {
    count = 0;
  }

  // Push a move on to the end of a move list.
  void push (const Move &m) {
    move[count++] = m;
  }

  // Push a move on to the end of a move list.
  void push
  (coord from, coord to, 
   Color color, Kind kind,
   Kind capture, Kind promote = NULL_KIND, 
   bool en_passant = false,
   Score score = 0) 
  {
    move[count++] = 
      Move (from, to, color, kind, capture, promote, en_passant, score);
  }

  // Access elements with the [] operator.
  Move &operator[] (uint8 i) {
    assert (i < count);
    return move[i];
  }

  // Push a move on to the end of a move list.
  Move operator[] (uint8 i) const {
    assert (i < count);
    return move[i];
  }

  // Sort a move vector using insertion sort.
  void sort (Score *keys) {
    for (int i = 1; i < count; i++)
      {
        Move index_elt = move[i];
        Score index_key = keys[i];
        int j = i;
        while (j > 0 && keys[j - 1] < index_key)
          {
            move[j] = move[j - 1];
            keys[j] = keys[j - 1];
            j = j - 1;
          }
        move[j] = index_elt;
        keys[j] = index_key;
      }
  }
  
  Move move[SIZE];
  uint8 count;
};

inline uint32
count (const Move_Vector &moves) {
  return moves.count;
}

#endif // _MOVE_
