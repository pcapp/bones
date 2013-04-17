CC=g++
CFLAGS=-std=c++11
INCLUDE_DIRS=-I"C:\Dev\libxml2-2.9.0\include"
LIBDIRS=-L"C:\Dev\libxml2-2.9.0\.libs"
LIBS=-lxml2
main: Main.cpp
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $(LIBDIRS) -o Main  Main.cpp $(LIBS)