#include <iostream>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "AnimCore.h"

using namespace std;

using glm::vec3;
using glm::vec4;
using glm::mat4;
using glm::quat;

void printMatrix(mat4 &m) {
	// Matrices stored in column-major order. GLM also uses column matrices.
	for(int y = 0; y < 4; ++y) {
		for(int x = 0; x < 4; ++x) {
			cout << m[x][y] << " ";
		}
		cout << endl;
	}
}

mat4 getChildToParentMatrix(const Joint &joint) {
	mat4 rotM(glm::mat4_cast(joint.orientation));
	mat4 transM = glm::translate(mat4(1.0f), joint.position);
	
	return transM * rotM;
	//return rotM * transM;
}

mat4 jointToWorld(const Skeleton &skeleton, const Joint &joint) {
	mat4 P(1.0);
	const Joint *pJoint = &joint;

	while(pJoint) {
		if(pJoint->parentIndex > -1) {
			cout << pJoint->name << " to " << skeleton.joints[pJoint->parentIndex].name << endl;
		} else {
			cout << pJoint->name << " to model" << endl;
		}

		mat4 P_next = getChildToParentMatrix(*pJoint);
		printMatrix(P_next);
		P = P_next * P;

		pJoint = (pJoint->parentIndex == -1) ? nullptr : &skeleton.joints[pJoint->parentIndex];
	}
		 
	return P;
}

void jointToWorldTest() {
	Joint j0, j1;

	// J0 Setup
	j0.name = "J0";
	j0.parentIndex = -1;
	j0.position = vec3(0, 0, 0);
	j0.orientation = quat(1, 0, 0, 0); // Identity quaternion

	// J1 Setup
	j1.name = "J1";
	j1.parentIndex = 0;
	j1.position.x = 70.1f;
	j1.position.y = 70.1f;
	j1.orientation = glm::quat_cast(glm::rotate(mat4(), 45.0f, vec3(0, 0, 1)));
	
	j1.name = "J2";
	j1.parentIndex = 0;
	j1.position.x = 70.1f;
	j1.position.y = 70.1f;
	j1.orientation = glm::quat_cast(glm::rotate(mat4(), 45.0f, vec3(0, 0, 1)));


	Skeleton skeleton;
	skeleton.joints.push_back(j0);
	skeleton.joints.push_back(j1);

	mat4 worldM = jointToWorld(skeleton, j1);

	cout << "World coordinate: " << worldM[3][0] << ", " << worldM[3][1] << ", " << worldM[3][2] << endl;
}

void childToParentTest() {
	Skeleton skeleton;
	Joint j0;

	j0.name = "J0";
	j0.parentIndex = -1;
	j0.position = vec3(70.71, 70.71, 0);
	j0.orientation = glm::quat_cast(glm::rotate(mat4(), 45.0f, vec3(0, 0, 1)));

	skeleton.joints.push_back(j0);

	mat4 P = getChildToParentMatrix(j0)  ;
	printMatrix(P);

	vec4 v(0,100, 0, 1); // Make it homogenous with w = 1
	vec4 v_ = P * v;

	cout << "In world space: " << v_.x << ", " << v_.y << ", " << v_.z << endl;
}

int main() {
	jointToWorldTest();
	//childToParentTest();

	return 0;
}