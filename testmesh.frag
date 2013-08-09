precision mediump float;

varying vec2 vTextureCoords;

uniform sampler2D tex0;

void main() {
	//gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
	gl_FragColor = texture2D(tex0, vTextureCoords);
}