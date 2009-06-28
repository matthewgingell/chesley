######################################################################
# Makefile for the Chesley the Chess Engine.			     #
# 								     #
# Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the #
# Chess Engine! is free software distributed under the terms of the  #
######################################################################

SHELL = /bin/sh

CXX = /opt/local/bin/g++-mp-4.4
OPT = -O3 -m64
INC = -Ideps
LIBS =
WARN = -Wall -Wextra

OBJS = $(subst .cpp,.o,$(SRCS))
SRCS = $(wildcard *.cpp)

VSN = -DSVN_REVISION=\"$(shell svnversion)\"

CXXFLAGS = -g $(OPT) $(ALG) $(INC) $(LIBS) $(VSN) $(WARN)

########################################################################
#                                                                      #
# Search configuration.                                                #
#                                                                      #
# The following macros are used to enable and disable various parts of #
# the tree search. Valid options are: ENABLE_ASPIRATION_WINDOW,	       #
# ENABLE_TRANS_TABLE, ENABLE_QSEARCH, ENABLE_NULL_MOVE, ENABLE_SEE,    #
# -DENABLE_PVS, -DENABLE_LMR					       #
#                                                                      #
########################################################################

ALG = -DENABLE_QSEARCH -DENABLE_PVS -DENABLE_ASPIRATION_WINDOW	\
-DENABLE_TRANS_TABLE -DENABLE_NULL_MOVE -DENABLE_EXTENSIONS	\
-DENABLE_LMR -DENABLE_SEE

################
# Main binary. #
################

all: chesley

chesley: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LIBS) -o $@

%.o : %.cpp *.hpp
	$(CXX) $(CXXFLAGS) -c $<

###############################
# Running games under xboard  #
###############################

XBOARD_OPTS = -debug -coords -size Giant -thinking -xponder -sgf out.pgn

# Launch an xboard session with Chesley.
play: chesley
	xboard -fcp ./chesley $(XBOARD_OPTS)

# Run an xboard game of Chesley against Chesley.
playself: chesley
	xboard -fcp ./chesley -scp ./chesley -mode TwoMachines $(XBOARD_OPTS)

# Run an xboard game of Chesley against the binary chesley.old.
playold: chesley
	xboard -st 0:01 -fcp ./chesley -scp ./chesley.old -mode TwoMachines $(XBOARD_OPTS) -mg 250

playother: chesley
#	xboard -st 0:01 -fcp ./chesley -scp ./amundsen -mg 100 -mode TwoMachines $(XBOARD_OPTS)
	xboard -st 0:01 -fcp ./chesley -scp ./other_binaries/zct -mg 1000 -mode TwoMachines $(XBOARD_OPTS)
#	xboard  -fcp ./chesley -scp ./other_binaries/pepitox -mg 1000 -mode TwoMachines $(XBOARD_OPTS)
#	xboard -st 0:01 -fcp ./chesley -scp ./tscp -mg 1000 -mode TwoMachines $(XBOARD_OPTS)
#	xboard -st 0:01 -fcp ./chesley -scp ./gray -mg 100 -mode TwoMachines $(XBOARD_OPTS)
#	xboard -st 0:01 -fcp ./chesley -scp gnuchess -mg 1000 -mode TwoMachines $(XBOARD_OPTS)
#	xboard -fcp ./olithink -scp gnuchess -mg 100 -mode TwoMachines $(XBOARD_OPTS)

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
	      Saturn* game.00* log.* logfile.* book.lrn position.bin	\
	      position.lrn *.d /cores/core.* *.fen \#*\# *.log log.0*	\
	      game.0* *mshark *.s
