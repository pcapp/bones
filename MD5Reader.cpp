#include <string>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <sstream>
#include <regex>
#include <glm/gtc/matrix_transform.hpp>

#include "AnimCore.h"
#include "MD5Reader.h"

using std::exception;
using std::runtime_error;
using std::string;
using std::ifstream;
using std::stringstream;
using std::endl;
using std::cout;
using std::regex;
using std::smatch;
using std::regex_match;
using glm::mat4;

void Md5Reader::processVersion() {	
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

void Md5Reader::processCommandLine() {
	string line;
	getline(mMeshFile, line);

	// Just by-passing this for now
}

void Md5Reader::processJointsAndMeshCounts() {
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

void Md5Reader::processJoints() {
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

mat4 Md5Reader::getChildToParentMatrix(const Joint &joint) {
	mat4 rotM(glm::mat4_cast(joint.orientation));
	mat4 transM = glm::translate(mat4(1.0f), joint.position);
	
	return transM * rotM;
}

void Md5Reader::computeJointToWorld(Joint &joint) {
	mat4 P(1.0);
	const Joint *pJoint = &joint;

	while(pJoint) {		
		mat4 P_next = getChildToParentMatrix(*pJoint);
		P = P_next * P;

		pJoint = (pJoint->parentIndex == -1) ? nullptr : &mJoints[pJoint->parentIndex];
	}
		 
	joint.jointToWorld = P;
}

void Md5Reader::computeWComponent(Joint &j) {
	// w = +/- sqrt(1 - x^2 - y^2 - z^2)
	// Convention dictates using the negative version
	float x = j.orientation.x;
	float y = j.orientation.y;
	float z = j.orientation.z;
	float w;

	float temp = 1.0f - x*x - y*y - z*z;
	if(temp < 0.0) {
		//cout << "Less than 0 case. (" << j.name << ") -sqrtf yields " << (-sqrtf(temp)) << endl;
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

void Md5Reader::buildJoint(const string &line, int count) {		
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

MD5_AnimInfo Md5Reader::parse(const string &meshFilename, const string &animFilename) {
	MD5_AnimInfo info;

	mMeshFile.open(meshFilename);

	if(!mMeshFile) {				
		throw runtime_error("Could not open the file.");
	} 

	mAnimFile.open(animFilename);
	if(!mAnimFile) {	
		throw runtime_error(string("Could not open ") + animFilename);
	}

	// Process the mesh data
	processVersion();
	processCommandLine();
	processJointsAndMeshCounts();
	processJoints();

	// Process the animation data
	processAnimHeader();
	processHierarchy();
	processBounds();
	processBaseframeJoints();
	processFramesData();
	
	info.baseframeJoints = mBaseframeJoints;
	info.jointsInfo = mJointsInfo;
	info.framesData = mFramesData;
	info.numFrames = mNumFrames;

	// temp
	info.joints = mJoints;

	return info;
}

void Md5Reader::processAnimHeader() {
	string line;
	stringstream tokens;
	int numJoints;

	getline(mAnimFile, line);
	tokens.str(line);

	string fieldName;
	int version;

	tokens >> fieldName >> version;
	if(version != 10) {
		throw runtime_error("Can only parse version 10");
	}

	// Command line
	getline(mAnimFile, line);
	
	// Blank line
	getline(mAnimFile, line);
	
	// Number of frames
	getline(mAnimFile, line);
	tokens.clear();
	tokens.str(line);
	tokens >> fieldName >> mNumFrames;
	mFramesData.resize(mNumFrames);
	
	// Number of joints
	getline(mAnimFile, line);
	tokens.clear();
	tokens.str(line);
	tokens >> fieldName >> numJoints;
	mJointsInfo.reserve(numJoints);
	mBaseframeJoints.reserve(numJoints);
	
	// Frame rate
	getline(mAnimFile, line);
	tokens.clear();
	tokens.str(line);
	tokens >> fieldName >> mFrameRate;
	
	// Number of animated components
	int numAnimatedComponents;
	getline(mAnimFile, line);
	tokens.clear();
	tokens.str(line);
	tokens >> fieldName >> numAnimatedComponents;
	
	for(auto &frameData : mFramesData) {
		frameData.resize(numAnimatedComponents);
	}
}

// TEMP
std::ostream &operator<<(std::ostream &out, const JointInfo &info) {
	out << info.name << " " << info.parent << " " << info.flags << " " << info.startIndex;
	return out;
}

void Md5Reader::processHierarchy() {
	string line;
	stringstream tokens;
	getline(mAnimFile, line);

	while(line == "") {
		getline(mAnimFile, line);
	}

	tokens.str(line);
	string fieldName;
	tokens >> fieldName;

	if(fieldName != "hierarchy") {
		throw runtime_error("File-out-of-order exception. hierarchy should come next.");
	}

	getline(mAnimFile, line);
	while(line != "}") {
		tokens.clear();
		tokens.str(line);

		JointInfo jointInfo;
		tokens >> jointInfo.name;
		tokens >> jointInfo.parent;
		tokens >> jointInfo.flags;
		tokens >> jointInfo.startIndex;

		mJointsInfo.push_back(jointInfo);
		
		getline(mAnimFile, line);
	}
}

void Md5Reader::processBounds() {
	string line;
	stringstream tokens;

	// Eat space
	do {
		getline(mAnimFile, line);
	} while(line == ""); 

	string fieldName;
	tokens.str(line);
	tokens >> fieldName;

	if(fieldName != "bounds") {
		cout << "Field name: " << fieldName << endl;
		throw runtime_error("Expected a bounds section." );
	}

	getline(mAnimFile, line);
	while(line != "}") {
		getline(mAnimFile, line);
	}
}


std::ostream &operator<<(std::ostream &out, const BaseframeJoint &j) {
	out << "(" << j.position.x << ", " << j.position.y << ", " << j.position.z << ") ";
	out << "[" << j.orientation.w << "(" << j.orientation.x << ", " << j.orientation.y << ", " << j.orientation.z << ")] ";

	return out;
}

void Md5Reader::processBaseframeJoints() {
	string line;
	stringstream tokens;

	// Eat space
	do {
		getline(mAnimFile, line);
	} while(line == ""); 

	string fieldName;
	tokens.str(line);
	tokens >> fieldName;

	if(fieldName != "baseframe") {
		throw runtime_error("Expected a baseframe section." );
	}

	getline(mAnimFile, line);
	string junk;
	unsigned count = 0;
	while(line != "}") {
		tokens.clear();
		tokens.str(line);
		BaseframeJoint joint;

		tokens >> junk; // (
		tokens >> joint.position.x;
		tokens >> joint.position.y;
		tokens >> joint.position.z;
		tokens >> junk; // )
		tokens >> junk; // (
		tokens >> joint.orientation.x;
		tokens >> joint.orientation.y;
		tokens >> joint.orientation.z;

		mBaseframeJoints.push_back(joint);
		//cout << joint << endl;

		getline(mAnimFile, line);
	}
}

void Md5Reader::processFramesData() {
	string line;
	stringstream tokens;
	unsigned frameCount = 0;

	while(frameCount < mNumFrames) {
		// Eat space
		do {
			getline(mAnimFile, line);
		} while(line == ""); 

		string fieldName;
		unsigned frameNumber;
		tokens.clear();
		tokens.str(line);
		tokens >> fieldName >> frameNumber;

		if(fieldName != "frame") {
			throw runtime_error("Expected a frame section." );
		}
		

		getline(mAnimFile, line);		
		std::vector<float> &frameData = mFramesData[frameNumber];
		unsigned count = 0;
		while(line != "}") {
			tokens.clear();
			tokens.str(line);

			float value;
			
			while(tokens >> value) {
				frameData[count++] = value;
			}

			getline(mAnimFile, line);
		}
	
		++frameCount;
	}
}