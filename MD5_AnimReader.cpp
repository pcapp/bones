#include "MD5_AnimReader.h"

#include <iostream>
#include <string>
#include <sstream>

using std::cout;
using std::endl;
using std::runtime_error;
using std::string;
using std::stringstream;

MD5_AnimInfo MD5_AnimReader::parse(const std::string &filename) {
	mAnimFile.open(filename);

	processAnimHeader();
	processHierarchy();
	processBounds();
	processBaseframeJoints();
	processFramesData();
	
	MD5_AnimInfo anim;

	anim.baseframeJoints = mBaseframeJoints;
	anim.jointsInfo = mJointsInfo;
	anim.framesData = mFramesData;
	anim.numFrames = mNumFrames;

	return anim;
}


void MD5_AnimReader::processAnimHeader() {
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

void MD5_AnimReader::processHierarchy() {
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

void MD5_AnimReader::processBounds() {
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

void MD5_AnimReader::processBaseframeJoints() {
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

void MD5_AnimReader::processFramesData() {
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