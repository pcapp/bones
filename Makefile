CC=g++
CFLAGS=-std=c++11

main: Main.cpp
	$(CC) -o Main $(CFLAGS) Main.cpp