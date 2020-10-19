#
# Typing 'make' or 'make rplc' will create the executable file.
#

# Define some Makefile variables for the compiler and compiler flags
# to use Makefile variables later in the Makefile: $()
#
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
#
CC = g++
CFLAGS  = -g -gdwarf-2

# typing 'make' will invoke the first target entry in the file 
# (in this case the default target entry)
# you can name this target entry anything, but "default" or "all"
# are the most commonly used names by convention
#
default: rplc

# To create the executable file we need the object files
rplc: rplc.o rplcLin.o
	$(CC) $(CFLAGS) -o rplc rplc.o rplcLin.o

# To create the object files, we need the source files:
rplc.o:  rplc.cpp rplc.h
	$(CC) $(CFLAGS) -c rplc.cpp

# To create the object file, we need the source files:
rplcLin.o:  rplcLin.cpp rplcLin.h
	$(CC) $(CFLAGS) -c rplcLin.cpp


# To start over from scratch, type 'make clean'.  This removes the executable file,
# as well as old .o object files and *~ backup files:
clean: 
	$(RM) rplc *.o *~

