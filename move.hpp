////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// move.hpp                                                                   //
//                                                                            //
// Representation and operations on a chess move.                             //
//                                                                            //
// Copyright Matthew Gingell <matthewgingell@gmail.com>, 2009-2015. Chesley   //
// the Chess Engine! is free software distributed under the terms of the      //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MOVE_
#define _MOVE_

#include "common.hpp"
#include "util.hpp"

///////////////////////
//   The Move type   //
///////////////////////

struct Board;

struct Move {

  // It's important we have a null constructor here as we create large arrays
  // of moves which we never access.
  Move () {}

  // Construct a move;
  Move (Coord from, Coord to, Color color, Kind kind, Kind capture,
        Kind promote = NULL_KIND, bool en_passant = false)
    : from (from), to (to), color (color), kind (kind),
      capture (capture), promote (promote), en_passant (en_passant) {}

  // Kind of color of the piece being moved.
  inline Color get_color () const {
    return color;
  }

  // Get the kind of the piece being moved.
  inline Kind get_kind () const {
    return kind;
  }

  // Returns the piece being captured, or NULL_KIND if this is not a capture.
  inline Kind get_capture () const {
    return capture;
  }

  // Returns the piece being promoted to, or NULL_KIND if this is not a
  // promotion.
  inline Kind get_promote () const {
    return promote;
  }

  // Is this move an en passant capture.
  inline bool is_en_passant () const {
    return en_passant;
  }

  // Is this a castling move?
  inline bool is_castle () const {
    return is_castle_qs () || is_castle_ks ();
  }

  // Is this a capture?
  inline bool is_capture () const {
    return capture != NULL_KIND;
  }

  // Is this a promotion?
  inline bool is_promote () const {
    return promote != NULL_KIND;
  }

  // Is this a king-side castle.
  inline bool is_castle_ks () const {
    return (kind == KING &&
            ((color == WHITE && from == E1 && to == G1) ||
             (color == BLACK && from == E8 && to == G8)));
  }

  // Is this a queen-side castle.
  inline bool is_castle_qs () const {
    return (kind == KING &&
            ((color == WHITE && from == E1 && to == C1) ||
             (color == BLACK && from == E8 && to == C8)));
  }

  // State
#if 0
  Coord  from       : 8;
  Coord  to         : 8;
  Color  color      : 8;
  Kind   kind       : 8;
  Kind   capture    : 8;
  Kind   promote    : 8;
  bool   en_passant : 8;
  uint32 unused     : 8;
#else
  Coord  from       : 6;
  Coord  to         : 6;
  Color  color      : 2;
  Kind   kind       : 4;
  Kind   capture    : 4;
  Kind   promote    : 4;
  bool   en_passant : 1;
  int    unused     : 5;
#endif
};

// A type to use for null moves.
const Move NULL_MOVE (0, 0, NULL_COLOR, NULL_KIND, NULL_KIND);

// Output for moves.
std::ostream & operator<< (std::ostream &os, const Move &b);

struct Undo {
  unsigned en_passant      :8;
  unsigned w_has_k_castled :1;
  unsigned w_has_q_castled :1;
  unsigned w_can_q_castle  :1;
  unsigned w_can_k_castle  :1;
  unsigned b_has_k_castled :1;
  unsigned b_has_q_castled :1;
  unsigned b_can_q_castle  :1;
  unsigned b_can_k_castle  :1;
  unsigned half_move_clock :16;
};

////////////////////////////////
//    The Move vector type.   //
////////////////////////////////

struct Move_Vector;

std::ostream &
operator<< (std::ostream &os, const Move_Vector &moves);

// Vector of moves.
struct Move_Vector {

  // Amazingly, examples of positions with 218 different possible moves exist.
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

  // Pop a move from the end of a move list.
  Move pop () {
    count--;
    return move[count + 1];
  }

  // Push a move on to the end of a move list.
  void push
  (Coord from, Coord to,
   Color color, Kind kind,
   Kind capture, Kind promote = NULL_KIND,
   bool en_passant = false)
  {
    move[count++] =
      Move (from, to, color, kind, capture, promote, en_passant);
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
  void sort (int32 *keys) {
    for (int i = 1; i < count; i++)
      {
        Move index_elt = move[i];
        int32 index_key = keys[i];
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
