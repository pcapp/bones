CC=/Users/petercappetto/emscripten/emcc
CFLAGS=-std=c++11 -stdlib=libc++
INC_DIRS=-I/usr/local/include
SRCS=main.cpp MD5Reader.cpp MD5_MeshReader.cpp
SHADERS=mesh.vert mesh.frag simple.vert simple.frag
PRELOADS=--preload-file Boblamp/boblampclean.md5mesh \
	--preload-file Boblamp/boblampclean.md5anim \
	--preload-file mesh.vert \
	--preload-file mesh.frag \
	--preload-file simple.vert \
	--preload-file simple.frag

main: $(SRCS)
	$(CC) $(CFLAGS) $(PRELOADS) -o test.html $(INC_DIRS) $(SRCS) 