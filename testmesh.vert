attribute vec3 VertexPosition;
attribute vec2 TextureCoords;

varying vec2 vTextureCoords;

uniform mat4 MVP;

void main() {
	gl_Position = MVP * vec4(VertexPosition, 1.0);
	vTextureCoords = TextureCoords;
}