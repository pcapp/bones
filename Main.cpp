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

int main(int argc, char **argv) {
	//glutInit(&argc, argv);
	//const string filename("boblampclean.md5mesh");
	//const string filename("test.txt");
	const string filename("Boblamp/boblampclean.md5mesh");
	Md5Reader reader;

	try {
		AnimInfo info = reader.parse(filename);
	}
	catch(exception &e) {
		cout << "Could not open the file: " << filename << endl;
		return -1;
	}
		
	// glutCreateWindow("Animated character");
	// glutDisplayFunc(render);
	// glutMainLoop();

	return 0;
}