CC=g++
CFLAGS=-std=c++11
INCLUDE_DIRS=-I"C:\Dev\libxml2-2.9.0\include" -I"C:\Dev\freeglut-2.8.0\include"
LIBDIRS=-L"C:\Dev\libxml2-2.9.0\.libs" -L"C:\Dev\freeglut-2.8.0\lib"
LIBS=-lxml2 -lfreeglut -lopengl32
main: Main.cpp
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $(LIBDIRS) -o Main  Main.cpp $(LIBS)