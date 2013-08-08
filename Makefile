CC=emcc
CFLAGS=-std=c++11 -stdlib=libc++
INC_DIRS=-I"C:\Dev\glm-0.9.3.4" -I"C:\Dev\freeglut-2.8.0\include" -I"C:\Dev\glew-1.9.0\include"
SRCS=animated_render.cpp Shader.cpp MD5_MeshReader.cpp MD5_AnimReader.cpp
PRELOADS=--preload-file Boblamp/boblampclean.md5mesh \
	--preload-file Boblamp/boblampclean.md5anim \
	--preload-file baseframe_shader.vert \
	--preload-file baseframe_shader.frag \
	--preload-file Skeleton.vert \
	--preload-file Skeleton.frag

main: $(SRCS)
	$(CC) $(CFLAGS) $(PRELOADS) -o test.html $(INC_DIRS) $(SRCS) 