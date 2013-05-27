#include <string>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <sstream>
#include <regex>
#include <glm/gtc/matrix_transform.hpp>

#include "AnimCore.h"
#include "MD5Reader.h"
#include "MD5_MeshReader.h"

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


MD5_VO Md5Reader::parse(const string &meshFilename, const string &animFilename) {
	MD5_VO vo;
	MD5_AnimInfo anim;
	MD5_MeshInfo mesh;

	// Process the mesh data
	MD5_MeshReader meshReader;
	mesh = meshReader.parse(meshFilename);

	mAnimFile.open(animFilename);
	if(!mAnimFile) {	
		throw runtime_error(string("Could not open ") + animFilename);
	}

	// Process the animation data
	processAnimHeader();
	processHierarchy();
	processBounds();
	processBaseframeJoints();
	processFramesData();
	
	anim.baseframeJoints = mBaseframeJoints;
	anim.jointsInfo = mJointsInfo;
	anim.framesData = mFramesData;
	anim.numFrames = mNumFrames;

	vo.mesh = mesh;
	vo.animations.push_back(anim);

	return vo;
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