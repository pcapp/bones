attribute vec3 VertexPosition;
attribute vec4 VertexRGBA;

varying vec4 RGBA;

uniform mat4 MVP;

void main() {
	gl_Position = MVP * vec4(VertexPosition, 1.0);
	RGBA = VertexRGBA;
}