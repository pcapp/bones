#!/bin/bash

CC=/Users/petercappetto/emscripten/emcc
SHADERS=(baseframe_shader.vert baseframe_shader.frag)
CMD="${CC}"

for shader in "${SHADERS[@]}"
do 
	CMD="${CMD} --preload-file ${shader}"
done

echo $CMD