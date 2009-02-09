#! /usr/bin/env ruby
#
# A simple chess playing algorithm.
#
# Matthew Gingell
# gingell@adacore.com
# December 2008

# Define a 2D square matrix type.
class Array2

  ######################################
  # Instance variables:                #
  #                                    #
  # @DIM: The dimention of the matrix. #
  # data: An array of matrix contents. #
  ######################################

  attr_reader :DIM
  attr_reader :data

  # Initialize a matrix, optionally with an Array or an Array2. Passed
  # data is always copied.
  def initialize(dim, initial_data = nil)
    @DIM = dim
    c = initial_data.class
    if (c == NilClass) then
      @data = Array.new(@DIM * @DIM)
    elsif (c == Array)
      @data = initial_data.dup
    elsif (c == Array2)
      @data = initial_data.data.dup
    end
  end

  # Fetch the element at (x, y).
  def [](x, y)
    @data[x + @DIM * y]
  end

  # Set the element at (x,y).
  def []=(x, y, rval)
    @data[x + @DIM * y] = rval
  end

  # Convert to a string.
  def to_s()
    s = ""
    for y in 0 .. @DIM - 1
      for x in 0 .. @DIM - 1
        v = self[x, y]
        s << (v == :e ? "." : v.to_s) << " "
      end
      s << "\n"
    end
    return s
  end

end # class Array2

# Define a game state type.
class Game_State


  ################################################################
  # Instance variables:                                          #
  #                                                              #
  # @board: An 8x8 matrix of pieces.                             #
  # @to_move: Which color is to move next.                       #
  # @status: A color if somebody has won, otherwise nil or draw. #
  ################################################################

  attr_reader :board
  attr_reader :to_move 
  attr_reader :status
  
  def initialize(to_move = :white, positions = nil)
    if !positions then
      @board = Array2.new(DIM, Initial_Positions)
    else
      @board = positions
    end
    @to_move = to_move
    @status = nil
  end

  # Build a new state by moving piece at "from" to "to".
  def child_state(from_x, from_y, to_x, to_y)
    new_board = Array2.new(DIM, @board)
    new_board[to_x, to_y] = @board[from_x, from_y]
    new_board[from_x, from_y] = :e
    return Game_State.new(other_color(@to_move), new_board)
  end
  
  #####################################################
  # Constants for setting up the initial board state. #
  #####################################################

  DIM = 8
  NSQUARES = DIM * DIM
  
  Initial_Positions =
    # 0   1   2   3   4   5   6   7
    [:r, :n, :b, :q, :k, :b, :n, :r,   # 0
     :p, :p, :p, :p, :p, :p, :p, :p,   # 1
     :e, :e, :e, :e, :e, :e, :e, :e,   # 2
     :e, :e, :e, :e, :e, :e, :e, :e,   # 3
     :e, :e, :e, :e, :e, :e, :e, :e,   # 4
     :e, :e, :e, :e, :e, :e, :e, :e,   # 5
     :P, :P, :P, :P, :P, :P, :P, :P,   # 6
     :R, :N, :B, :Q, :K, :B, :N, :R]   # 7
  
#    DIM = 3             
#    Initial_Positions = 
#      [:e, :p, :k,        
#       :e, :e, :e,        
#       :K, :e, :e]       
  
    
#   DIM = 5              
#   Initial_Positions =  
#     [:r, :n, :b, :k, :b, 
#      :p, :p, :p, :p, :p, 
#      :e, :e, :e, :e, :e, 
#      :P, :P, :P, :P, :P, 
#      :R, :N, :B, :K, :B] 
  

  ############################################
  # Constant vectors describing legal moves. #
  ############################################

  Rook_Moves   = [[ 0, 1], [ 0,-1], [ 1, 0], [-1, 0]]

  Knight_Moves = [[ 2, 1], [ 1, 2], [-1, 2], [-2, 1],
                  [-2,-1], [-1,-2], [ 1,-2], [ 2,-1]]

  Bishop_Moves = [[ 1, 1], [ 1,-1], [-1,-1], [-1, 1]]
  
  Queen_Moves  = [[ 1, 0], [ 1, 1], [ 0, 1], [-1, 1],
                  [-1, 0], [-1,-1], [ 0,-1], [ 1,-1]]

  King_Moves   = [[ 1, 0], [ 1, 1], [ 0, 1], [-1, 1],
                  [-1, 0], [-1,-1], [ 0,-1], [ 1,-1]]

  ###############
  # Conversions #
  ###############

  # Convert to a human readable string.
  def to_s
    @to_move.to_s << " to move:\n" << @board.to_s
  end

  ##############
  # Accessors  #
  ##############

  # Return a matrix of piece locations
  def board
    return @board
  end

  # Return the state of a location. Either :while, :black, or e:
  def color(x, y)
    case @board[x, y]
    when :P, :R, :N, :B, :Q, :K
      return :white
    when :p, :r, :n, :b, :q, :k
      return :black
    when :e
      return :e
    end
  end

  # Return the opposite of a color.
  def other_color(c)
    case c
    when :white 
      return :black
    when :black 
      return :white
    else 
      raise "Fatal error!"
    end
  end

  # Iterate over the game board.
  def each_square
    for x in 0 .. DIM - 1
      for y in 0 .. DIM - 1
        yield x, y
      end
    end
  end
  
  ##############
  # Predicates #
  ##############

  # Is a location on the chess board?
  def in_bounds?(x, y)
    x >= 0 and x < DIM and y >= 0 and y < DIM 
  end

  # Is a location empty?
  def is_empty?(x, y)
    return @board[x, y] == :e
  end

  # Is a location occupied by a piece of the current player's own
  # color.
  def own_color?(x,y)
    return color(x, y) == @to_move
  end

  # Is a location occupied by a piece of the other player's color?
  def other_color?(x, y)
    return color(x, y) == other_color(@to_move)
  end

  def our_king_in_check?
    # Temporarily flip @to_move so we can reuse the logic from
    # other_king_in_check?
    @to_move = other_color(@to_move)
    rv = other_king_in_check?
    @to_move = other_color(@to_move)
    return rv
  end

  # Check whether the other color's king is threatened state.
  def other_king_in_check?
    
    # Which king to check?
    other_king = (@to_move == :white) ? :k : :K

    # Find it.
    x, y = each_square {|x,y| break x, y if @board[x, y] == other_king}

    # Is this square threatened by a pawn?
    dir = (@to_move == :white) ? 1 : -1
    for dx,dy in [[-1, dir], [1, dir]]
      nx, ny = x + dx, y + dy
      if in_bounds?(nx, ny) and own_color?(nx, ny) and is_pawn?(nx, ny)
        return true
      end
    end

    # Is this square threatened by a knight?
    for dx, dy in Knight_Moves
      nx, ny = x + dx, y + dy
      if in_bounds?(nx, ny) and own_color?(nx, ny) and is_knight?(nx, ny)
      then
        return true
      end
    end
      
    # Check for rook, bishop, and queen moves
    for dx, dy in Queen_Moves
      dist = 1
      while true
        nx, ny = x + dx * dist, y + dy * dist

        # We're done searching in this direction if we are out of
        # bounds or we've have found a piece of the other color.
        if !in_bounds?(nx, ny) or other_color?(nx, ny) then
          break
        end

        # If we find a piece of the our own color, see if it can move
        # in the direction we are currently checking. (In fact in the
        # opposite direction, but the moves in question are all
        # symmetric.)

        if own_color?(nx, ny) then
          return true if is_rook?(nx, ny) and Rook_Moves.member?([dx,dy])
          return true if is_bishop?(nx, ny) and Bishop_Moves.member?([dx,dy])
          return true if is_queen?(nx, ny)
        end

        dist += 1
      end
    end
      
    # Is this square threatened by a king?
    for dx, dy in King_Moves
      nx, ny = x + dx, y + dy
      if in_bounds?(nx, ny) and own_color?(nx, ny) and is_king?(nx, ny)
        return true 
      end
    end

    # If we get here, the other king is not threatened.
    return false
  end

  # Predicates for testing piece types.
  def is_pawn?(x, y) 
    @board[x,y] == :P or @board[x,y] == :p
  end
  
  def is_rook?(x, y) 
    @board[x,y] == :R or @board[x,y] == :r
  end

  def is_knight?(x, y) 
    @board[x,y] == :N or @board[x,y] == :n
  end

  def is_bishop?(x, y) 
    @board[x,y] == :B or @board[x,y] == :b
  end

  def is_queen?(x, y) 
    @board[x,y] == :Q or @board[x,y] == :q
  end

  def is_king?(x, y) 
    @board[x,y] == :K or @board[x,y] == :k
  end

  ##############################
  # Enumerating possible moves #
  ##############################

  # Enumerate moves for a piece at (x,y).
  def each_move(x, y)
    case @board[x, y]
    when :p, :P then each_pawn_move(x, y)   {|c| yield c}
    when :r, :R then each_rook_move(x, y)   {|c| yield c}
    when :n, :N then each_knight_move(x, y) {|c| yield c}
    when :b, :B then each_bishop_move(x, y) {|c| yield c}
    when :q, :Q then each_queen_move(x, y)  {|c| yield c}
    when :k, :K then each_king_move(x, y)   {|c| yield c}
    else raise "Fatal error!"
    end
  end

  # Enumerate moves for a pawn at (x, y).
  def each_pawn_move(x, y)
    # Which direction is this pawn moving?
    dir = @to_move == :white ? -1 : 1

    # Move a pawn diagonally if it can take a piece.
    for dx, dy in [[1,dir], [-1,dir]] do
      nx, ny = x + dx, y + dy
      if in_bounds?(nx, ny) and other_color?(nx, ny)
        yield child_state(x, y, nx, ny)
      end
    end

    # Move a pawn one space forwards.
    nx, ny = x, y + dir
    if in_bounds?(nx, ny) and is_empty?(nx, ny)
      yield child_state(x, y, nx, ny)
    end

    # Move a pawn two space forwards if it hasn't moved yet.
    if ((y == 1 and @to_move == :black) or
        (y == DIM - 2 and @to_move == :white)) and
        is_empty?(x, y + 2 * dir)
    then
      new_state = child_state(x, y, x, y + 2 * dir)
      yield new_state if new_state != nil
    end
  end

  # Enumerate moves given a set of direction vectors. Pieces which
  # move an indefinite number of spaces in a set of directions (R,B,Q)
  # all share the same logic here.
  def each_dxdy_move(x, y, dirs)

    # For each direction.
    for dx, dy in dirs
      dist = 1
      while true
        nx, ny = x + dx * dist, y + dy * dist

        # We're done searching in this direction if we are out of
        # bounds or we've have found a piece of our own color.
        if !in_bounds?(nx, ny) or own_color?(nx, ny) then
          break
        end

        # If we find a piece of the other color, take it and then
        # we're done.
        if other_color?(nx, ny) then
          yield nx, ny
          break
        end

        # Yield and continue searching if square is empty.
        if is_empty?(nx, ny)
          yield nx, ny
          dist += 1
        end
      end
    end
  end

  # Enumerate moves for a rook at (x,y).
  def each_rook_move(x, y)
    each_dxdy_move(x, y, Rook_Moves) do |nx, ny|
      yield child_state(x, y, nx, ny)
    end
  end

  # Enumerate moves for a bishop at (x,y).
  def each_bishop_move(x, y)
    each_dxdy_move(x, y, Bishop_Moves) do |nx, ny|
      yield child_state(x, y, nx, ny)
    end
  end

  # Enumerate moves for a queen at (x,y).
  def each_queen_move(x, y)
    each_dxdy_move(x, y, Queen_Moves) do |nx, ny|
      yield child_state(x, y, nx, ny)
    end
  end

  # Enumerate moves for a knight at (x,y).
  def each_knight_move(x, y)
    for dx, dy in Knight_Moves
      nx, ny = x + dx, y + dy
      if in_bounds?(nx, ny) and !own_color?(nx, ny) then
        yield child_state(x, y, nx, ny)
      end
    end
  end

  # Enumerate moves for a king at (x,y).
  def each_king_move(x, y)
    for dx, dy in King_Moves
      nx, ny = x + dx, y + dy
      if in_bounds?(nx, ny) and !own_color?(nx, ny) then
        yield child_state(x, y, nx, ny)
      end
    end
  end

  # Yield each possible child state for this position.
  def each_child_state
    move_count = 0
    each_square do |x, y|
      if own_color?(x,y) 
        each_move(x, y) do |c|
          # If our own king, which is the other king in the child
          # state, is in check then we can not return this move.
          if !c.other_king_in_check? then
            yield c
            move_count += 1
          end
        end
      end
    end

    if move_count == 0 then
      # If we have no legal moves and our king is threatened, then we
      # have lost, otherwise we have a draw.
      if our_king_in_check? 
        @status = other_color(@to_move)
      else 
        @status = :draw
      end
    end
  end

end # Class Game_State

# For an initial 8x8 we can generate 3 ply in 19 seconds.
# (about 9402 positions.) That's a rate of about 500 moves 
# per second. That's about 4 million instructs per move...
#
# 4 ply is pretty hopeless... (tons of pawn moves at the
# beginning of the game is a killer.)

$count = 0
def print_tree(state = Game_State.new, d = 0, parent = nil)
  print "#{$count} positions generated."
  $count += 1
  print "depth = #{d}\n"  
  print parent.to_s
  print "==>\n"
  print state.to_s
  state.each_child_state do |child|
    print_tree(child, d + 1, state) if d < 3
  end
  print "status: #{state.status}\n" if state.status != nil
  print "\n\n"
end

print_tree

# gingell$ ruby -rprofile ./chess.rb > out
#   %   cumulative   self              self     total
#  time   seconds   seconds    calls  ms/call  ms/call  name
#  20.92    36.02     36.02    44344     0.81     2.83  Array#each
#  18.27    67.47     31.45   674322     0.05     0.07  Array2#[]
#   9.89    84.49     17.02   290608     0.06     0.09  Game_State#in_bounds?
#   7.97    98.22     13.73    48966     0.28     2.91  Range#each
#   5.52   107.72      9.50  1303157     0.01     0.01  Fixnum#+
#   5.35   116.93      9.21  1217114     0.01     0.01  Array#[]
#   3.88   123.61      6.68   901550     0.01     0.01  Fixnum#*
#   3.28   129.26      5.65   156928     0.04     0.13  Game_State#own_color?
#   2.98   134.39      5.13   222904     0.02     0.09  Game_State#color
#   2.90   139.38      4.99   683404     0.01     0.01  Symbol#==
#   2.52   143.72      4.34   576259     0.01     0.01  Fixnum#<
#   2.24   147.57      3.85   574598     0.01     0.01  Fixnum#>=
#   1.71   150.51      2.94    65976     0.04     0.15  Game_State#other_color?
#   1.61   153.28      2.77     9402     0.29    11.16  Game_State#other_king_in_check?
#   1.27   155.46      2.18   271396     0.01     0.01  Array#length
#   1.17   157.48      2.02   271396     0.01     0.01  Kernel.kind_of?
#   1.13   159.43      1.95   278393     0.01     0.01  Fixnum#==
#   0.63   160.52      1.09    18804     0.06     0.08  Array2#[]=
#   0.60   161.56      1.04     9402     0.11     0.59  Game_State#child_state
#   0.51   162.43      0.87   116058     0.01     0.01  String#<<
#   0.48   163.25      0.82   124840     0.01     0.01  BasicObject#!
#   0.47   164.06      0.81     9403     0.09     0.15  Array2#initialize
#   0.45   164.83      0.77     3368     0.23    15.95  Game_State#each_pawn_move
#   0.43   165.57      0.74    75378     0.01     0.01  Game_State#other_color
#   0.27   166.04      0.47    18806     0.02     0.12  Class#new
#   0.25   166.47      0.43     6736     0.06    13.03  Game_State#each_move
#   0.21   166.84      0.37    70978     0.01     0.01  Fixnum#-
#   0.16   167.11      0.27    28208     0.01     0.01  Module#==
#   0.15   167.36      0.25     7498     0.03     0.10  Game_State#is_empty?
#   0.13   167.58      0.22     9403     0.02     0.03  Game_State#initialize
#   0.10   167.76      0.18    18804     0.01     0.01  Array#[]=
#   0.10   167.94      0.18     9403     0.02     0.03  Kernel.dup
#   0.07   168.06      0.12    27753     0.00     0.00  Symbol#to_s
#   0.05   168.15      0.09     3469     0.03     0.04  BasicObject#!=
#   0.05   168.23      0.08     9403     0.01     0.01  Array#initialize_copy
#   0.05   168.31      0.08     9403     0.01     0.01  Kernel.class
#   0.04   168.38      0.07      421     0.17    36.34  Object#print_tree
#   0.03   168.44      0.06      842     0.07    16.21  Game_State#each_knight_move
#   0.03   168.49      0.05     2526     0.02     0.02  Kernel.print
#   0.03   168.54      0.05     2105     0.02     4.76  Game_State#each_dxdy_move
#   0.03   168.59      0.05      841     0.06    10.64  Array2#to_s
#   0.03   168.64      0.05     3469     0.01     0.01  BasicObject#==
#   0.02   168.68      0.04      841     0.05    10.71  Game_State#to_s
#   0.02   168.71      0.03      421     0.07    32.02  Game_State#each_square
#   0.02   168.74      0.03      842     0.04     4.22  Game_State#each_bishop_move
#   0.01   168.75      0.01      421     0.02    34.35  Game_State#each_child_state
#   0.01   168.76      0.01      842     0.01     4.33  Game_State#each_rook_move
#   0.01   168.77      0.01     2526     0.00     0.00  IO#write
#   0.01   168.78      0.01      842     0.01     0.01  Fixnum#to_s
#   0.00   168.78      0.00        2     0.00     0.00  IO#set_encoding
#   0.00   168.78      0.00        1     0.00     0.00  NilClass#to_s
#   0.00   168.78      0.00      421     0.00     6.79  Game_State#each_queen_move
#   0.00   168.78      0.00      421     0.00     7.74  Game_State#each_king_move
#   0.00   168.78      0.00        5     0.00     0.00  Module#attr_reader
#   0.00   168.78      0.00       38     0.00     0.00  Module#method_added
#   0.00   168.78      0.00        2     0.00     0.00  Class#inherited
#   0.00   168.78      0.00      133     0.00     0.30  Game_State#is_rook?
#   0.00   168.78      0.00      133     0.00     0.23  Game_State#is_bishop?
#   0.00   168.78      0.00      133     0.00     0.00  Game_State#is_queen?
#   0.00   172.17      0.00        1     0.00 172170.00  #toplevel
