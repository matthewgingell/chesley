######################################################################
# Makefile for the Chesley the Chess Engine.			     #
# 								     #
# Copyright Matthew Gingell <matthewgingell@gmail.com>, 2015. Chesley the #
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

#CXX=/Users/gingell/projects/mingw/prefix/bin/i386-mingw32-g++
#CXX=i386-mingw32-g++
#EXT=.exe

OPT = -m64 -O3
PROF =
INC = -Ideps
LIBS =
WARN = -Wall -Wextra
OBJS = $(subst .cpp,.o,$(SRCS))
SRCS = $(wildcard *.cpp)
CXXFLAGS = $(OPT) $(PROF) $(ALG) $(INC) $(VSN) $(WARN) $(DEBUG)
DEBUG = -g3 # -DTRACE_EVAL

#########################################################################
# 								        #
# Search configuration.                                                 #
#  								        #
# The following macros are used to enable and disable various parts of  #
# the tree search. Valid options are:				        #
# 								        #
# ENABLE_ASPIRATION_WINDOW					        #
# ENABLE_EXTENSIONS						        #
# ENABLE_FUTILITY						        #
# ENABLE_LMR							        #
# ENABLE_NULL_MOVE						        #
# ENABLE_PVS							        #
# ENABLE_SEE							        #
# ENABLE_TRANS_TABLE						        #
# 								        #
#########################################################################

ALG = -DENABLE_ASPIRATION_WINDOW -DENABLE_FUTILITY -DENABLE_NULL_MOVE	\
-DENABLE_PVS -DENABLE_SEE -DENABLE_TRANS_TABLE -DENABLE_LMR		\
-DENABLE_EXTENSIONS

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

#XBOARD_OPTS = -debug -coords -autoflag -size Giant -thinking -xponder	\
-sgf out.pgn

XBOARD_OPTS = -debug -coords -autoflag -size Large -thinking -xponder	\
-sgf out.pgn

# Launch an xboard session with Chesley.
play: chesley
	xboard -fcp ./chesley $(XBOARD_OPTS)

# Run an xboard game of Chesley against Chesley.
playself: chesley
	rm -f out.pgn
	xboard -tc 1 -inc 0  -mg 10 -fcp ./chesley -scp ./chesley -mode	\
TwoMachines $(XBOARD_OPTS)

# Generate a pgn file of games against different engines.
out.pgn:
	make playother

playold: chesley chesley.old
	rm -f out.pgn
	xboard -tc 1 -mps 60 -mg 1 -fcp ./chesley -scp ./chesley.old -mode	\
TwoMachines $(XBOARD_OPTS)

# Run an xboard game of Chesley against another engine.
playother: chesley
	rm -f out.pgn

# Annotate out.pgn using Crafty.
annotate: out.pgn
	printf "smpmt 2\nannotate out.pgn Chesley 1-999 1 .5\n" | other_binaries/crafty

#############
# Clean up. #
#############

clean :
	rm -rf chesley *.exe *.inc a.out *.o *.dSYM *~ TAGS *.gcda	\
	      a.out Saturn* game.00* log.* logfile.* book.lrn		\
	      position.bin position.lrn *.d /cores/core.* *.fen \#*\#	\
	      *.log log.0* game.0* *mshark *.s out.pgn
