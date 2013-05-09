#version 150

in vec3 VertexPosition;
in vec4 VertexRGBA;
out vec4 RGBA;

void main() {
	gl_Position = vec4(VertexPosition, 1.0);
	RGBA = VertexRGBA;
}