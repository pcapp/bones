attribute vec3 VertexPosition;
attribute vec4 JointIndices;
attribute vec4 JointWeights;

uniform mat4 MVP;
uniform mat4 MatrixPalette[40];

void main() {
	vec4 finalPosition = vec4(0.0, 0.0, 0.0, 0.0);

	for(int i = 0; i < 4; ++i) {
		float weight = JointWeights[i];

		if(weight > 0.0) {
			int jointIndex = int(JointIndices[i]);
			vec4 tempPos = MatrixPalette[jointIndex] * vec4(VertexPosition, 1.0);
			finalPosition += weight * tempPos;
		}
	}

	gl_Position = MVP * finalPosition;
}