#version 150

in vec4 RGBA;
out vec4 Color;

void main() {
	Color = RGBA;
	//Color = vec4(1.0, 0.0, 0.0, 1.0f);
}