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
	while(mFile.good() && line != "}") {
		buildJoint(line, count);
		getline(mFile, line);		
	}
}


std::ostream & operator<<(std::ostream &out, Joint &joint) {
	out << joint.name << " ";
	out << joint.position.x << ", " << joint.position.y << ", " << joint.position.z;
	out << "(TODO orientation)";

	return out;
}

void Md5Reader::buildJoint(const string &line, int count) {		
	Joint j;
	stringstream tokens(line);
	string dump;

	tokens >> j.name;
	tokens >> j.parentIndex;
	tokens >> dump; // (
	tokens >> j.position.x;
	tokens >> j.position.y;
	tokens >> j.position.z;

	// TODO Handle the orientation here.

	cout << j << endl;
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
	
	return info;
}