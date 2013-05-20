#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

using namespace std;
using namespace glm;

struct Joint {
	Joint *parent;
	vec3 position;
	quat orientation;
};

ostream &operator<<(ostream &out, const vec3 &v) {
	return out << "< " << v.x << ", " << v.y << ", " << v.z << ">";
}

ostream &operator<<(ostream &out, const vec4 &v) {
	return out << "< " << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ">";
}

void printMatrix(const mat4 &m) {
	for(int row = 0; row < 4; ++row) {
		for(int col = 0; col < 4; ++col) {
			cout << m[col][row] << " ";
		}
		cout << endl;
	}
}

mat4 getModelSpaceTransform(const Joint &joint) {
	mat4 rotM = glm::mat4_cast(joint.orientation);
	mat4 transM = glm::translate(mat4(1.0f), joint.position);

	mat4 T = transM * rotM;

	const Joint *parent = joint.parent;

	while(parent) {
		// Prepend the transform dawg
		rotM = glm::mat4_cast(parent->orientation);
		transM = glm::translate(mat4(1.0f), parent->position);

		T = transM * rotM * T;

		parent = parent->parent;
	}

	return T;
}


void test() {
	Joint j0 = {nullptr, vec3(10, 10, 0), quat_cast(rotate(mat4(), 45.0f, vec3(0, 0, 1)))};
	Joint j1 = {&j0, vec3(0, 14.1421, 0), quat_cast(rotate(mat4(), -45.0f, vec3(0, 0, 1)))};
	Joint j2 = {&j1, vec3(-10, -10, 0), quat_cast(rotate(mat4(), 90.0f, vec3(0, 0, 1)))};

	mat4 T = getModelSpaceTransform(j2);
	printMatrix(T);
}

void testJointToModel() {
	Joint j0 = {nullptr, vec3(10, 10, 0), quat_cast(rotate(mat4(), 45.0f, vec3(0, 0, 1)))};
	Joint j1 = {&j0, vec3(0, 14.1421, 0), quat_cast(rotate(mat4(), -45.0f, vec3(0, 0, 1)))};
	Joint j2 = {&j1, vec3(-10, -10, 0), quat_cast(rotate(mat4(), 90.0f, vec3(0, 0, 1)))};

	mat4 T = getModelSpaceTransform(j2);
	vec4 point(0, -20, 0, 1);
	vec4 result = T * point;

	cout << result << endl;
}

int main() {
	test();
	testJointToModel();

	return 0;
}