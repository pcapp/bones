#include <GL/glew.h>
#if __APPLE__
	#include <GLUT/glut.h>
#else 
	#include <GL/glut.h>
#endif

#include <exception>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

#include "AnimCore.h"
#include "MD5Reader.h"

using namespace std;

AnimInfo g_AnimInfo;

GLuint hProgram;
GLuint hVertShader;
GLuint hFragShader;
GLuint vbo;
GLuint vao;
GLuint hIndexBuffer;

bool buildShader(GLuint &hShader, const char *filename, GLuint shaderType) {
	ifstream in(filename);

	if(!in) {
		cout << "Could not open " << filename << endl;
		return false;
	}
	
	stringstream inStream;
	inStream << in.rdbuf() << '\0';

	int bufSize = inStream.str().length();
	char *buffer = new char[bufSize];
	memcpy(buffer, inStream.str().c_str(), bufSize);
	
	const GLchar *sources[] = {buffer};
	
	hShader = glCreateShader(shaderType);
	glShaderSource(hShader, 1, sources, NULL);
	glCompileShader(hShader);

	int compileStatus;
	glGetShaderiv(hShader, GL_COMPILE_STATUS, &compileStatus);

	if(GL_FALSE == compileStatus) {
		cout << "Could not compile the shader" << endl;
		GLint logLength;
		glGetShaderiv(hShader, GL_INFO_LOG_LENGTH, &logLength);
		if(logLength > 0) {
			GLsizei numBytesRead;
			char *log = new char[logLength];
			glGetShaderInfoLog(hShader, logLength, &numBytesRead, log);
			cout << log << endl;
			delete [] log;
		}
		delete[] buffer;
		return false;
	}

	glAttachShader(hProgram, hShader);
	
	delete[] buffer;
	return true;
}

bool makeShaderProgram() {
	hProgram = glCreateProgram();

	if(!buildShader(hVertShader, "simple.vert", GL_VERTEX_SHADER)) {		
		return false;
	}

	glBindAttribLocation(hProgram, 0, "VertexPosition");
	glBindAttribLocation(hProgram, 1, "VertexRGBA");

	if(!buildShader(hFragShader, "simple.frag", GL_FRAGMENT_SHADER)) {
		return false;
	}

	GLint linkStatus;
	glLinkProgram(hProgram);
	glGetProgramiv(hProgram, GL_LINK_STATUS, &linkStatus);
	if(GL_FALSE == linkStatus) {
		cout << "Error linking the shader program." << endl;
		GLint logLength;
		glGetProgramiv(hProgram, GL_INFO_LOG_LENGTH, &logLength);

		if(logLength > 0) {
			GLsizei bytesRead;
			char *log = new char[logLength];
			glGetProgramInfoLog(hProgram, logLength, &bytesRead, log);	
			cout << log << endl;	
			delete []log;
		}

		return false;
	}

	return true;
}

void render() {
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

int main(int argc, char **argv) {
	Md5Reader reader;
	const string filename("Boblamp/boblampclean.md5mesh");

	try {
		g_AnimInfo = reader.parse(filename);
	}
	catch(exception &e) {
		cout << e.what() << endl;
		return -1;
	}
	cout << "Parsed " << filename << " successfully." << endl; 

	glutInit(&argc, argv);
	glutInitWindowSize(640, 480);
	glutCreateWindow("Animated character");	
	// Context created at this point
	glewExperimental = GL_TRUE; 
	GLenum glewRetVal = glewInit();
	if(!makeShaderProgram()) {
		cout << "Unable to build the shader program." << endl;
		return -1;
	}

	glutDisplayFunc(render);
	glutMainLoop();

	return 0;
}