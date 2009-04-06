##############################################
# Makefile for the Chesley the Chess Engine. #
# 	                                     #
# Matthew Gingell			     #
# gingell@adacore.com			     #
##############################################

SHELL = /bin/sh

CXX = g++-mp-4.4
OPT = -O3 -march=native
INC = -Ideps
LIBS =
CXXFLAGS = -g -Wall -m64 $(OPT) $(ALG) $(INC) $(LIBS)
OBJS = $(subst .cpp,.o,$(SRCS))
SRCS = $(wildcard *.cpp)

########################################################################
# Search configuration.						       #
# 								       #
# The following macros are used to enable and disable various parts of #
# the tree search. Valid options are: ENABLE_ASPIRATION,	       #
# ENABLE_TRANS_TABLE, ENABLE_QSEARCH, ENABLE_NULL_MOVE,		       #
# ENABLE_ORDER_MOVES, ENABLE_ALPHA_BETA, ENABLE_SEE		       #
########################################################################

ALG = -DENABLE_ALPHA_BETA -DENABLE_ORDER_MOVES -DENABLE_TRANS_TABLE	\
-DENABLE_QSEARCH -DENABLE_NULL_MOVE -DENABLE_ASPIRATION_WINDOW		\
-DENABLE_SEE

################
# Main binary. #
################

all: chesley

chesley: $(OBJS) vsn.o
	$(CXX) $(CXXFLAGS) $(OBJS) vsn.o $(LIBS) -o $@

# Create an object containing the version number from svn info.
vsn.o: *.cpp *.hpp
	svn info -r HEAD | sed -n 's/Revision: \([0-9]*\)/const char	\
	*SVN_REVISION = \"\1\";/p' > vsn.xxx
	$(CXX) -x c++ -c $(CXXFLAGS) vsn.xxx -o $@

%.o : %.cpp *.hpp
	$(CXX) $(CXXFLAGS) -c $<

###############################
# Running games under xboard  #
###############################

XBOARD_OPTS = -debug -coords -size Large -xponder -thinking

# Launch an xboard session with Chesley.
play: chesley
	xboard -fcp ./chesley $(XBOARD_OPTS)

# Run an xboard game of Chesley against Chesley.
playself: chesley 
	xboard -fcp ./chesley -scp ./chesley -mode TwoMachines $(XBOARD_OPTS)

# Run an xboard game of Chesley against the binary chesley.old.
playold: chesley
	xboard -fcp ./chesley -scp ./chesley.old -mode TwoMachines

#######################
# Regression testing. #
#######################

# Run the test suites.
check: chesley
	./run_tests.rb

#############
# Clean up. #
#############

clean :
	rm -rf chesley *.inc a.out *.o *.dSYM *~ TAGS *.gcda a.out	\
	      Saturn* game.00* log.00* book.lrn position.bin		\
	      position.lrn *.d /cores/core.* *.pgn *.fen \#*\# *.log	\
	      vsn.xxx
