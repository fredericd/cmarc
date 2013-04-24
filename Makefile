CC=g++
CFLAGS=''
LIBS=''

all: test

test: test.o
	$(CC) -o test test.o

test.o: test.cpp cmarc.hpp
	$(CC) -o test.o -c test.cpp
