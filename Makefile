# define the compiler to use
CC = g++

# define any compile-time flags 
CFLAGS = -std=c++11 -Wall -g

# define any directories containing header files other than /usr/include
INCLUDES = 

# define library paths in addition to /usr/lib
LFLAGS = 

# define any libraries to link into executable
LIBS =

# define the C++ source files
SRCS = codegenerator.cpp error.cpp symbol.cpp scanner.cpp parser.cpp main.cpp

OBJS = $(SRCS:.cpp=.o)

# define the executable file 
MAIN = compiler

all:    $(MAIN)

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

main.o: main.cpp parser.h
	$(CC) $(CFLAGS) $(INCLUDES) -c main.cpp
	
scanner.o: scanner.cpp scanner.h symbol.h
	$(CC) $(CFLAGS) $(INCLUDES) -c scanner.cpp
	
parser.o: parser.cpp parser.h scanner.cpp scanner.h error.h error.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c parser.cpp
	
symbol.o: symbol.cpp symbol.h
	$(CC) $(CFLAGS) $(INCLUDES) -c symbol.cpp
	
error.o: error.cpp error.h
	$(CC) $(CFLAGS) $(INCLUDES) -c error.cpp
	
codegenerator.o: codegenerator.cpp codegenerator.h symbol.h scanner.h
	$(CC) $(CFLAGS) $(INCLUDES) -c codegenerator.cpp
	
# Phony targets
.PHONY: clean
clean:
	$(RM) *.o *~ $(MAIN)
