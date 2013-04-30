#include <string>
#include <iostream>
#include <exception>
#include <fstream>
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

void Md5Reader::checkVersion() {
	string line;
	getline(mFile, line); 
	regex r("^MD5Version [[:digit::]]+$");
	smatch m;

	regex_match(line, m, r);
	cout << m.size() << endl;
}

AnimInfo Md5Reader::parse(const string &filename) {
	AnimInfo info;
	
	cout << "Opening: " << filename << endl;
	mFile.open(filename);

	if(!mFile) {		
		throw exception("Could not open the file.");
	} 

	checkVersion();

	return info;
}