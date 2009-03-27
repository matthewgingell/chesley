# Makefile for the Chesley chess engine.
#
# Matthew Gingell
# gingell@adacore.com

SHELL = /bin/sh

LIBS =
OBJS = $(subst .cpp,.o,$(SRCS))
SRCS = $(wildcard *.cpp)

# Turn on and off various parts of the search algorithm. Options are:
#
# ENABLE_ASPIRATION, ENABLE_TRANS_TABLE, ENABLE_QSEARCH,
# ENABLE_NULL_MOVE, ENABLE_ORDER_MOVES, ENABLE_ALPHA_BETA

ALG = -DENABLE_ALPHA_BETA -DENABLE_ORDER_MOVES -DENABLE_TRANS_TABLE	\
-DENABLE_QSEARCH -DENABLE_ASPIRATION_WINDOW -DENABLE_NULL_MOVE

LIBS =
CXX = g++-mp-4.4
#CXXFLAGS = -g -Wall -m64 -O3 $(ALG)
CXXFLAGS = -g -Wall $(ALG)

all: chesley

# Build the chesley binary.
chesley: vsn.inc $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LIBS) -o $@

# Create the version number file from svn info.
vsn.inc: *.cpp *.hpp
	svn info -r HEAD |  \
	sed -n 's/Revision: \([0-9]*\)/#define SVN_REVISION \"\1\"/p' > $@
	touch session.cpp

# Run an xboard game with chesley.
play: chesley
	xboard -debug -fcp ./chesley -coords -size Huge

# Run an xboard game of Chesley against Chesley.
playself: chesley 
	xboard -debug -fcp ./chesley -scp ./chesley -coords -size Small	\
	-mode TwoMachines

# Run an xboard game of Chesley against chesley.old.
playold: chesley
	xboard -debug -fcp ./chesley -scp ./chesley.old -coords -size	\
	Small -mode TwoMachines

# Run the test suites.
check: chesley
	./run_tests.rb

%.o : %.cpp *.hpp
	$(CXX) $(CXXFLAGS) -c $<

clean :
	rm -rf chesley *.inc a.out *.o *.dSYM *~ TAGS *.gcda a.out	\
	      Saturn* game.00* log.00* book.lrn position.bin		\
	      position.lrn *.d /cores/core.* *.pgn *.fen \#*\#
