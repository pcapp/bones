#include <iostream>
#include <sstream>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "MD5Reader.h"

using namespace std;
using glm::mat4;
using glm::vec4;

vector<BaseframeJoint> baseframe;
vector<JointInfo> hierarchy;

void printMatrix(mat4 &m) {
	// Matrices stored in column-major order. GLM also uses column matrices.
	for(int y = 0; y < 4; ++y) {
		for(int x = 0; x < 4; ++x) {
			cout << m[x][y] << " ";
		}
		cout << endl;
	}
}

ostream &operator<<(ostream &out, const BaseframeJoint &j) {
	out << "<" << j.position.x << ", " << j.position.y << ", " << j.position.z << "> ";
	out << "   [" << j.orientation.w << " (" << j.orientation.x << ", " << j.orientation.y << ", " << j.orientation.z << ")]";
	return out;
}

ostream &operator<<(ostream &out, const JointInfo &i) {
	out << i.name;
	return out;
}

ostream &operator<<(ostream &out, const vec4 &v) {
	out << "<" << v.x << ", " << v.y << ", " << v.z << ">";
	return out;
}

BaseframeJoint makeBaseframeJointFromLine(const string &line) {
	BaseframeJoint j;
	stringstream tokens(line);
	string junk;

	float x, y, z;

	tokens >> junk;
	tokens >> j.position.x >> j.position.y >> j.position.z;
	tokens >> junk; // )
	tokens >> junk ; // (
	tokens >> x >> y >> z;
	tokens >> junk; // )

	float w = 0.0f;
	float term = 1.0f - x*x - y*y - z*z;
	
	if(term > 0) {
		w = -sqrtf(term);
	}

	j.orientation.w = w;
	j.orientation.x = x;
	j.orientation.y = y;
	j.orientation.z = z;

	return j;
}

JointInfo makeJointInfo(const string &line ) {
	JointInfo info;
	stringstream tokens(line);
	string junk;

	tokens >> info.name >> info.parent >> info.flags >> info.startIndex;

	return info;
}

mat4 jointToModel(int index) {
	mat4 P(1.0f);

	int curIndex = index;

	while(curIndex > -1) {
		BaseframeJoint &j = baseframe[curIndex];
		JointInfo &i = hierarchy[curIndex];

		mat4 rotM = glm::mat4_cast(j.orientation);
		mat4 transM = glm::translate(mat4(1.0f), j.position);
		mat4 toParent = transM * rotM;

		cout << i.name;
		if(i.parent > -1) {
			cout << " to  ";
		} else {
			cout << " to model" << endl;
		}

		//cout << "Rotation: " << endl;
		//printMatrix(rotM);
		//cout << "Translation: " << endl;
		//printMatrix(transM);

		P = toParent * P;
		curIndex = i.parent;
	}
	cout << endl;

	return P;
}
 
int main() {
	// Target values
	//"origin"	-1 ( -0.000000 0.016430 -0.006044 ) ( 0.707107 0.000000 0.707107 )		// 
	//"sheath"	0 ( 11.004813 -3.177138 31.702473 ) ( 0.307041 -0.578614 0.354181 )		// origin
	//"sword"	1 ( 9.809593 -9.361549 40.753730 ) ( 0.305557 -0.578155 0.353505 )		// sheath
	
	BaseframeJoint origin = makeBaseframeJointFromLine("( -0.000000 0.016430 -0.006044 ) ( 0.707107 0.000242 0.707107 )");
	BaseframeJoint sheath = makeBaseframeJointFromLine("( 31.228901 6.251943 9.236629 ) ( -0.022398 0.133633 0.852233 )");
	BaseframeJoint sword = makeBaseframeJointFromLine("( 0.003848 -11.026810 0.100900 ) ( 0.001203 -0.000819 0.001677 )");

	baseframe.push_back(origin);
	baseframe.push_back(sheath);
	baseframe.push_back(sword);

	JointInfo originInfo = makeJointInfo("\"origin\"	-1 63 0	//");
	JointInfo sheathInfo = makeJointInfo("\"sheath\"	0 63 6	// origin");
	JointInfo swordInfo = makeJointInfo("\"sword\"	1 63 12	// sheath");

	hierarchy.push_back(originInfo);
	hierarchy.push_back(sheathInfo);
	hierarchy.push_back(swordInfo);

	mat4 j2m = jointToModel(0);
	vec4 translation = j2m[3];
	glm::quat rot = glm::quat_cast(j2m);

	cout << translation << " [(" << rot.w << " (" << rot.x << ", " << rot.y << ", " << rot.z << ")]";
	
	return 0;
}