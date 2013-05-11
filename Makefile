CC = g++
OPT = -O3
#OPT = -g
WARN = -Wall
CFLAGS = $(OPT) $(WARN) $(INC) $(LIB)

# List all your .cc files here (source files, excluding header files)
ILP_SRC = ilp.cpp

# List corresponding compiled object files here (.o files)
ILP_OBJ = ilp.o
 
#################################

# default rule

all: ilp
	@echo "my work is done here..."


# rule for making ilp

ilp: $(ILP_OBJ)
	$(CC) -o ilp $(CFLAGS) $(ILP_OBJ) -lm
	@echo "-----------DONE WITH SIM_CACHE-----------"


# generic rule for converting any .cc file to any .o file
 
.cc.o:
	$(CC) $(CFLAGS)  -c $*.cc


# type "make clean" to remove all .o files plus the sim binary

clean:
	rm -f *.o ilp


# type "make clobber" to remove all .o files (leaves sim binary)

clobber:
	rm -f *.o
