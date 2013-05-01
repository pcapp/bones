#include <GL/glut.h>
#include <exception>
#include <string>
#include <iostream>
#include "AnimCore.h"
#include "MD5Reader.h"

using namespace std;

void render() {
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
}

void read_file_test() {
	//ifstream file("C:\\Users\\Peter\\Desktop\\MOVE\\build\\Debug\\Boblamp\\boblampclean.md5mesh");
	ifstream file("Boblamp/boblampclean.md5mesh");

	if(!file) {
		cout << "Could not read the file." << endl;
	}

	while(!file.eof()) {
		string line;
		getline(file, line);
		cout << line << endl;
	}
}

void parse_test() {
	Md5Reader reader;

	try {
		reader.parse("Boblamp/boblampclean.md5mesh");
	}
	catch(exception &e) {
		cout << e.what() << endl;
	}

	cout << "Parsed successfully." << endl;
}

int main(int argc, char **argv) {
	//read_file_test();
	parse_test();

	return 0;
}