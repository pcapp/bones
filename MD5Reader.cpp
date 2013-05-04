#include <string>
#include <iostream>
#include <exception>
#include <sstream>
#include <regex>

#include "AnimCore.h"
#include "MD5Reader.h"

using std::exception;
using std::string;
using std::ifstream;
using std::stringstream;
using std::endl;
using std::cout;
using std::regex;
using std::smatch;
using std::regex_match;

void Md5Reader::processVersion() {	
	string line;
	
	getline(mFile, line); 
	stringstream sstream(line);
	string versionStr;
	int versionNum;

	sstream >> versionStr;
	sstream >> versionNum;

	if(versionNum != 10) {
		throw exception("Only processing version 10 of the MD5 spec.");
	}
}

void Md5Reader::processCommandLine() {
	string line;
	getline(mFile, line);

	// Just by-passing this for now
}

void Md5Reader::processJointsAndMeshCounts() {
	string line;
	
	// Eat spaces
	do {
		getline(mFile, line);
	} while(mFile && line == "");

	if(!mFile) {
		throw exception("EOF reached and no joint count found.");
	}

	stringstream tokens(line);
	string numJointsStr;
	int numJoints = -1;

	tokens >> numJointsStr;
	tokens >> numJoints;
	mJoints.resize(numJoints);

	// Get the mesh info
	do {
		getline(mFile, line);
	} while(mFile && line == "");

	if(!mFile) {
		throw exception("EOF reached and no mesh count found.");
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
		getline(mFile, line);
	} while(mFile && line == "");

	stringstream tokens(line);
	string type;

	tokens >> type;
	// Should be "joints"
	if(type != "joints") {
		throw exception("Expected a joints block.");
	}

	int count = 0;
	getline(mFile, line); // Advance to the next line
	while(mFile.good() && line != "}") {
		buildJoint(line, count++);
		getline(mFile, line);		
	}
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
	float w, x, y, z;

	tokens >> j.name;
	tokens >> j.parentIndex;
	tokens >> dump;
	tokens >> j.position.x;
	tokens >> j.position.y;
	tokens >> j.position.z;
	tokens >> dump;
	tokens >> dump;
	tokens >> x;
	tokens >> y;
	tokens >> z;

	// We must calculate w.
	// w = +/- sqrt(1 - x^2 - y^2 - z^2)
	// Convention dictates using the negative version
	float temp = 1.0f - x*x - y*y - z*z;
	if(temp < 0.0) {
		//cout << "Less than 0 case. (" << j.name << ") -sqrtf yields " << (-sqrtf(temp)) << endl;
		w = 0.0f;
	} else {
		w = -sqrtf(temp);
	}

	j.orientation.x = x;
	j.orientation.y = y;
	j.orientation.z = z;

	mJoints[count] = j;	
}

AnimInfo Md5Reader::parse(const string &filename) {
	AnimInfo info;

	mFile.open(filename);

	if(!mFile.is_open()) {				
		throw exception("Could not open the file.");
	} 

	processVersion();
	processCommandLine();
	processJointsAndMeshCounts();
	processJoints();

	info.skeleton.joints = mJoints;
	
	return info;
}