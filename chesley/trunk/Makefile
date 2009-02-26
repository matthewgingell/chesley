SHELL = /bin/sh

LIBS =
OBJS = $(subst .cpp,.o,$(SRCS))
SRCS = $(wildcard *.cpp)

CXX = g++ 
#CXX = g++-mp-4.4

CXXFLAGS = -g -Wall -static
#CXXFLAGS = -g -Wall -m64
#CXXFLAGS = -g -Wall -m64 -O3 -funroll-all-loops -DUSE_TRANS_TABLE
#CXXFLAGS = -g -Wall -m64 -O3 -funroll-all-loops -DNDEBUG -DUSE_TRANS_TABLE

all: chesley

chesley: $(OBJS) *.hpp
	$(CXX) $(CXXFLAGS) $(OBJS) $(LIBS) -o $@

play: chesley
	xboard -debug -fcp ./chesley  -coords -size Medium

playself: chesley
	xboard -debug -fcp ./chesley -scp ./chesley -coords -size	\
	Medium -mode TwoMachines

playold: chesley
	xboard -debug -fcp ./chesley -scp ./chesley.old -coords -size	\
	Medium -mode TwoMachines

check: chesley
	./run_tests.rb

%.o : %.cpp *.hpp
	$(CXX) $(CXXFLAGS) -c $<

clean :
	rm -rf chesley a.out *.o *.dSYM *~ TAGS *.gcda a.out Saturn*	\
	      game.00* log.00* book.lrn position.bin position.lrn *.d	\
	      /cores/core.* *.pgn *.fen \#*\#
