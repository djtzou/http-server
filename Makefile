#
# 'make'        build executable file 'http-server'
# 'make clean'  removes all .o and executable files
#

# Define the C compiler to use
CC = gcc

# Compiler flags
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS = -g -Wall

# Define any directories containing header files other than /usr/include
INCLUDES = -I./utils  -I./threadpool -I./server

# Define any libraries to link into executable.
# -pthread adds support for multithreading with the pthreads library. This option sets flags for both the preprocessor and linker.
LIBS = -pthread

# Define the C source files
SRCS = server/server.c server/request.c threadpool/threadpool.c utils/inet_sockets.c utils/error_functions.c utils/utils.c

# Define the C object files
#
# This uses Suffix Replacement within a macro:
#   $(name:string1=string2)
#         For each word in 'name' replace 'string1' with 'string2'
# Below we are replacing the suffix .c of all words in the macro SRCS
# with the .o suffix
#
OBJS = $(SRCS:.c=.o)

# Define the executable file
MAIN = http-server

# Build the executable
.PHONY: clean

# By default, build the first target 'all'
all:	$(MAIN)
		@echo  server has been compiled

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LIBS)

# Define the remove command
RM = -rm -f

# This is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file)
# (see the gnu make manual section about automatic variables)
.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o server/*.o threadpool/*.o utils/*.o *~ $(MAIN)
