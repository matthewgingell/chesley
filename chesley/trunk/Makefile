SHELL = /bin/sh

LIBS = 
OBJS = $(subst .cpp,.o,$(SRCS))
SRCS = $(wildcard *.cpp)

#CXX = g++ 
CXX = g++-mp-4.4

#CXXFLAGS = -g -Wall -m64
CXXFLAGS = -g -Wall -m64 -O3 -funroll-all-loops -DNDEBUG

all: chesley

chesley: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LIBS) -o $@

play: chesley
	xboard -debug -fcp ./chesley -size Large

check: chesley
	./run_tests.rb

%.o : %.cpp *.hpp
	$(CXX) $(CXXFLAGS) -c $<

clean :
	rm -rf chesley a.out *.o *.dSYM *~ TAGS *.gcda a.out Saturn*	\
	      game.00* log.00* book.lrn position.bin position.lrn *.d	\
	      /cores/core.* *.pgn *.fen \#*\#
