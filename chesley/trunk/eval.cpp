#include "bits64.hpp"
#include "board.hpp"
#include "eval.hpp"

/***************************************************************/
/* The values used in the following table were lifted from the */
/* sources to Crafty by Dr. Robert M Hyatt.                    */
/* Please see http://www.craftychess.com.                      */
/***************************************************************/

// Look up with piece, color, and square.
static int
simple_positional_value[5][2][64] = {
  
  {
    /* White Pawn */

    { 0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0, -12, -12,   0,   0,   0,
      1,   1,   1,  10,  10,   1,   1,   1,
      3,   3,   3,  13,  13,   3,   3,   3,
      6,   6,   6,  16,  16,   6,   6,   6,
     10,  10,  10,  30,  30,  10,  10,  10,
     70,  70,  70,  70,  70,  70,  70,  70,
      0,   0,   0,   0,   0,   0,   0,   0 },

    /* Black Pawn */

    { 0,   0,   0,   0,   0,   0,   0,   0,
      70,  70,  70,  70,  70,  70,  70,  70,
      10,  10,  10,  30,  30,  10,  10,  10,
      6,   6,   6,   16,  16,   6,   6,   6,
      3,   3,   3,   13,  13,   3,   3,   3,
      1,   1,   1,   10,  10,   1,   1,   1,
      0,   0,   0,  -12, -12,   0,   0,   0,
      0,   0,   0,    0,   0,   0,   0,   0 }
  },

  {
    /* White Rook */

    { 0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0 },

    /* Black Rook */

    { 0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0 }
  },

  {
    /* White Knights */

    {-20, -20, -20, -20, -20, -20, -20, -20,
       0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,  16,  14,  14,  16,   0,   0,
       0,  10,  18,  20,  20,  18,  10,   0,
       0,  12,  20,  24,  24,  20,  12,   0, 
       0,  12,  20,  24,  24,  20,  12,   0,
       0,  10,  16,  20,  20,  16,  10,   0,
     -30, -20, -20, -10, -10, -20, -20, -30 },
  

    /* Black Knights */

   {-30, -20, -20, -10, -10, -20, -20, -30,
      0,  10,  16,  20,  20,  16,  10,   0,
      0,  12,  20,  24,  24,  20,  12,   0,
      0,  12,  20,  24,  24,  20,  12,   0,
      0,  10,  18,  20,  20,  18,  10,   0, 
      0,   0,  16,  14,  14,  16,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
    -20, -20, -20, -20, -20, -20, -20, -20}
  },

  {
    /* White Bishops */

   {-10, -10,  -8,  -6,  -6,  -8, -10, -10,
      0,   8,   6,   8,   8,   6,   8,   0,
      2,   6,  12,  10,  10,  12,   6,   2,
      4,   8,  10,  16,  16,  10,   8,   4,
      4,   8,  10,  16,  16,  10,   8,   4,
      2,   6,  12,  10,  10,  12,   6,   2,
      0,   8,   6,   8,   8,   6,   8,   0,
      0,   0,   2,   4,   4,   2,   0,   0},

  /* Black Bishops */

  {  0,   0,   2,   4,   4,   2,   0,   0,
     0,   8,   6,   8,   8,   6,   8,   0,
     2,   6,  12,  10,  10,  12,   6,   2,
     4,   8,  10,  16,  16,  10,   8,   4,
     4,   8,  10,  16,  16,  10,   8,   4,
     2,   6,  12,  10,  10,  12,   6,   2,
     0,   8,   6,   8,   8,   6,   8,   0,
   -10, -10,  -8,  -6,  -6,  -8, -10, -10 }
  },

  {
    /* White Queens. */

    {0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   4,   4,   4,   4,   0,   0,
     0,   4,   4,   6,   6,   4,   4,   0,
     0,   4,   6,   8,   8,   6,   4,   0,
     0,   4,   6,   8,   8,   6,   4,   0,
     0,   4,   4,   6,   6,   4,   4,   0,
     0,   0,   4,   4,   4,   4,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0},

    /* Black Queens. */

    {0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   4,   4,   4,   4,   0,   0,
     0,   4,   4,   6,   6,   4,   4,   0,
     0,   4,   6,   8,   8,   6,   4,   0,
     0,   4,   6,   8,   8,   6,   4,   0,
     0,   4,   4,   6,   6,   4,   4,   0,
     0,   0,   4,   4,   4,   4,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0}
  }
};

// Evaluate a the positional strength of a based based on the
// preceding table.
int 
eval_simple_positional (Board b) {
  const int WHITE_IDX = 0;
  const int BLACK_IDX = 1;
  int bonus = 0;

  for (int ci = WHITE_IDX; ci <= BLACK_IDX; ci++)
    for (int ki = PAWN; ki <= QUEEN; ki++)
      {
	Color c = (ci == WHITE_IDX ? WHITE : BLACK);
	Kind k = (Kind) ki;
	bitboard pieces = 
	  b.kind_to_board (k) & 
	  b.color_to_board (c);
	
	while (pieces)
	  {
	    int idx = bit_idx (pieces);
	    bonus += (int) c * 
	      simple_positional_value[k][ci][idx];
	    pieces = clear_lsb (pieces);
	  }
    }

  return bonus;
}
