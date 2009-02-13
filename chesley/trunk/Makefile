SHELL = /bin/sh

CXX = g++
#CXX = g++-mp-4.4

CXXFLAGS = -g -Wall
#CXXFLAGS = -g -Wall -m64 -O3 -funroll-all-loops
#CXXFLAGS = -g -Wall -m64 -O3 -funroll-all-loops -DNDEBUG

LIBS = 
OBJECTS = board.o eval.o genmasks.o genmoves.o gentables.o main.o	\
 search.o session.o xboard.o test.o

all: chesley

chesley: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LIBS) -o $@

play: chesley
	xboard -debug -fcp ./chesley -size Medium

tourn: chesley
	xboard -debug -fcp ./chesley -scp `which gnuchess` \ 
	   matchGames 5 -size Medium

%.o : %.cpp *.hpp
	$(CXX) $(CXXFLAGS) -c $<

clean :
	rm -rf chesley a.out *.o *.dSYM *~ TAGS *.gcda a.out Saturn*	\
	      game.00* log.00* book.lrn position.bin position.lrn *.d	\
	      /cores/core.* *.pgn *.fen
