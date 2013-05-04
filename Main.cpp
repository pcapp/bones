#include <GL/glut.h>
#include <exception>
#include <string>
#include <iostream>
#include "AnimCore.h"
#include "MD5Reader.h"

using namespace std;

AnimInfo g_AnimInfo;

void render() {
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

int main(int argc, char **argv) {
	Md5Reader reader;

	try {
		g_AnimInfo = reader.parse("Boblamp/boblampclean.md5mesh");
	}
	catch(exception &e) {
		cout << e.what() << endl;
		return -1;
	}

	glutInit(&argc, argv);
	glutInitWindowSize(640, 480);
	glutCreateWindow("Animated character");	
	glutDisplayFunc(render);
	glutMainLoop();

	return 0;
}