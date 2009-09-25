######################################################################
# Makefile for the Chesley the Chess Engine.			     #
# 								     #
# Copyright Matthew Gingell <gingell@adacore.com>, 2009. Chesley the #
# Chess Engine! is free software distributed under the terms of the  #
# GNU Public License.                                                #
######################################################################

######################
# Building for POSIX #
######################

CXX=g++

########################
# Building for Windows #
########################

#CXX=/Users/gingell/projects/mingw/prefix/bin/x86_64-pc-mingw32-c++
#CXX=/opt/local/bin/i386-mingw32-g++
#EXT=.exe

OPT = -g3 -O3 -m64
PROF =
INC = -Ideps
LIBS = 
WARN = -Wall -Wextra
OBJS = $(subst .cpp,.o,$(SRCS))
SRCS = $(wildcard *.cpp)
VSN = -DSVN_REVISION=\"$(shell svnversion)\"
CXXFLAGS = $(OPT) $(PROF) $(ALG) $(INC) $(VSN) $(WARN) $(DEBUG)

########################################################################
#                                                                      #
# Search configuration.                                                #
#                                                                      #
# The following macros are used to enable and disable various parts of #
# the tree search. Valid options are: ENABLE_ASPIRATION_WINDOW,	       #
# ENABLE_TRANS_TABLE, ENABLE_QSEARCH, ENABLE_NULL_MOVE, ENABLE_SEE,    #
# -DENABLE_PVS, -DENABLE_LMR, -DENABLE_FUTILITY, -DENABLE_RAZORING     #
#                                                                      #
########################################################################

ALG = -DENABLE_QSEARCH -DENABLE_PVS -DENABLE_ASPIRATION_WINDOW	\
-DENABLE_NULL_MOVE -DENABLE_TRANS_TABLE -DENABLE_FUTILITY	\
-DENABLE_RAZORING -DENABLE_EXTENSIONS -DENABLE_LMR -DENABLE_SEE

DEBUG = # -DTRACE_EVAL

################
# Main binary. #
################

all: chesley

chesley: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LIBS) -o $@$(EXT)

%.o : %.cpp *.hpp
	$(CXX) $(CXXFLAGS) -c $<

###############################
# Running games under xboard  #
###############################

XBOARD_OPTS = -debug -coords -autoflag -size Giant -thinking -xponder	\
-sgf out.pgn

# Launch an xboard session with Chesley.
play: chesley
	xboard -fcp ./chesley $(XBOARD_OPTS)

# Run an xboard game of Chesley against Chesley.
playself: chesley
	xboard -tc 1 -mps 100 -fcp ./chesley -scp ./chesley -mode TwoMachines $(XBOARD_OPTS)

# Run an xboard game of Chesley against another engine.
playother: chesley
	xboard -tc 1 -mg 1000 -fcp ./chesley -scp other_binaries/zct	\
-mode TwoMachines $(XBOARD_OPTS)

#############
# Clean up. #
#############

clean :
	rm -rf chesley *.exe *.inc a.out *.o *.dSYM *~ TAGS *.gcda	\
	      a.out Saturn* game.00* log.* logfile.* book.lrn		\
	      position.bin position.lrn *.d /cores/core.* *.fen \#*\#	\
	      *.log log.0* game.0* *mshark *.s
