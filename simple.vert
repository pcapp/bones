#version 150

in vec3 VertexPosition;
in vec4 VertexRGBA;
out vec4 RGBA;

uniform mat4 MVP;

void main() {
	gl_Position = MVP * vec4(VertexPosition, 1.0);
	RGBA = VertexRGBA;
}