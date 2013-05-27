#include "MD5_MeshReader.h"

#include <fstream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using glm::mat4;

using std::runtime_error;
using std::string;
using std::stringstream;
using std::getline;

MD5_MeshInfo MD5_MeshReader::parse(const std::string &filename) {
	mMeshFile.open(filename);

	if(!mMeshFile) {				
		throw runtime_error("Could not open the file.");
	} 

	MD5_MeshInfo mesh;

	processVersion();
	processCommandLine();
	processJointsAndMeshCounts();
	processJoints();

	mMeshFile.close();

	return mesh;
}

void MD5_MeshReader::processVersion() {	
	string line;
	
	getline(mMeshFile, line); 
	stringstream sstream(line);
	string versionStr;
	int versionNum;

	sstream >> versionStr;
	sstream >> versionNum;

	if(versionNum != 10) {
		throw runtime_error("Only processing version 10 of the MD5 spec.");
	}
}

void MD5_MeshReader::processCommandLine() {
	string line;
	getline(mMeshFile, line);

	// Just by-passing this for now
}

void MD5_MeshReader::processJointsAndMeshCounts() {
	string line;
	
	// Eat spaces
	do {
		getline(mMeshFile, line);
	} while(mMeshFile && line == "");

	if(!mMeshFile) {
		throw runtime_error("EOF reached and no joint count found.");
	}

	stringstream tokens(line);
	string numJointsStr;
	int numJoints = -1;

	tokens >> numJointsStr;
	tokens >> numJoints;
	mJoints.resize(numJoints);

	// Get the mesh info
	do {
		getline(mMeshFile, line);
	} while(mMeshFile && line == "");

	if(!mMeshFile) {
		throw runtime_error("EOF reached and no mesh count found.");
	}

	
	tokens.clear();
	tokens.str(line);
	
	string numMeshesStr;
	int numMeshes = -1;

	tokens >> numMeshesStr;
	tokens >> numMeshes;
}

void MD5_MeshReader::processJoints() {
	string line;

	do {
		getline(mMeshFile, line);
	} while(mMeshFile && line == "");

	stringstream tokens(line);
	string type;

	tokens >> type;
	// Should be "joints"
	if(type != "joints") {
		throw runtime_error("Expected a joints block.");
	}

	int count = 0;
	getline(mMeshFile, line); // Advance to the next line
	while(mMeshFile.good() && line != "}") {
		buildJoint(line, count++);
		getline(mMeshFile, line);		
	}
}

mat4 MD5_MeshReader::getChildToParentMatrix(const Joint &joint) {
	mat4 rotM(glm::mat4_cast(joint.orientation));
	mat4 transM = glm::translate(mat4(1.0f), joint.position);
	
	return transM * rotM;
}

void MD5_MeshReader::computeJointToWorld(Joint &joint) {
	mat4 P(1.0);
	const Joint *pJoint = &joint;

	while(pJoint) {		
		mat4 P_next = getChildToParentMatrix(*pJoint);
		P = P_next * P;

		pJoint = (pJoint->parentIndex == -1) ? nullptr : &mJoints[pJoint->parentIndex];
	}
		 
	joint.jointToWorld = P;
}

void MD5_MeshReader::computeWComponent(Joint &j) {
	// w = +/- sqrt(1 - x^2 - y^2 - z^2)
	// Convention dictates using the negative version
	float x = j.orientation.x;
	float y = j.orientation.y;
	float z = j.orientation.z;
	float w;

	float temp = 1.0f - x*x - y*y - z*z;
	if(temp < 0.0) {
		w = 0.0f;
	} else {
		w = -sqrtf(temp);
	}

	j.orientation.w = w;
}

std::ostream & operator<<(std::ostream &out, Joint &joint) {
	out << joint.name << " ";
	out << joint.position.x << ", " << joint.position.y << ", " << joint.position.z;
	out << "[";
	out << joint.orientation.w << " (" 
		<< joint.orientation.x << ", " 
		<< joint.orientation.y << ", "
		<< joint.orientation.z << ")]";

	return out;
}

void MD5_MeshReader::buildJoint(const string &line, int count) {		
	Joint j;
	stringstream tokens(line);
	string dump;
	float orientW, orientX, orientY, orientZ;

	tokens >> j.name;
	tokens >> j.parentIndex;
	tokens >> dump;
	tokens >> j.position.x;
	tokens >> j.position.y;
	tokens >> j.position.z;
	tokens >> dump;
	tokens >> dump;
	tokens >> orientX;
	tokens >> orientY;
	tokens >> orientZ;

	j.orientation.x = orientX;
	j.orientation.y = orientY;
	j.orientation.z = orientZ;
	computeWComponent(j);

	computeJointToWorld(j);

	mJoints[count] = j;	
}