#!/bin/bash

CC=/Users/petercappetto/emscripten/emcc
SHADERS=(mesh.vert mesh.frag simple.vert simple.frag)
CMD="${CC}"

for shader in "${SHADERS[@]}"
do 
	CMD="${CMD} --preload-file ${shader}"
done

echo $CMD