CC=/Users/petercappetto/emscripten/emcc
CFLAGS=-std=c++11 -stdlib=libc++
INC_DIRS=-I/usr/local/include
SRCS=baseframe_render.cpp Shader.cpp MD5_MeshReader.cpp
SHADERS=baseframe_shader.vert baseframe_shader.frag
PRELOADS=--preload-file Boblamp/boblampclean.md5mesh \
	--preload-file Boblamp/boblampclean.md5anim \
	--preload-file baseframe_shader.vert \
	--preload-file baseframe_shader.frag

main: $(SRCS)
	$(CC) $(CFLAGS) $(PRELOADS) -o test.html $(INC_DIRS) $(SRCS) 