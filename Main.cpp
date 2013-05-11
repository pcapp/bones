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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "AnimCore.h"
#include "MD5Reader.h"

using namespace std;
using glm::mat4;
using glm::vec3;

////////////////////////
// GLOBALS
////////////////////////
AnimInfo g_AnimInfo;

GLuint hProgram;
GLuint hVertShader;
GLuint hFragShader;
GLuint vbo;
GLuint vao;
GLuint hIndexBuffer;

float yRotation = 0.0f;

mat4 model;
mat4 view;
mat4 projection;
mat4 MVP;

const int kTimerPeriod = 50;

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

	projection = glm::perspective(45.0f, 4.0f/3.0f, 0.1f, 100.0f);
	view = glm::translate(mat4(), vec3(0, 0, -5));
	
	return true;
}

void setUpModel() {
	float zPos = 0.0f;

	GLfloat points[] = {
		-0.75f, 0.0f, zPos, 1.0f, 0.0f, 0.0f, 1.0f,
		0.75f, 0.0f, zPos, 0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 0.75f, zPos, 0.0f, 0.0f, 1.0f, 1.0f 
	};

	// Set up the vertex buffer object
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);

	// Set up the vertex array object
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glEnableVertexAttribArray(0); // Vertex position
	glEnableVertexAttribArray(1); // RGBA
	GLsizei stride = 7 * sizeof(GLfloat);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void *)(3 * sizeof(GLfloat)));

	// Set up the indices
	GLushort indices[3] = {0, 1, 2};
	glGenBuffers(1, &hIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices) * sizeof(GLushort), indices, GL_STATIC_DRAW);

	glPointSize(5.0f);
}

void render() {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(hProgram);
	MVP = projection * view * model;
	GLint location = glGetUniformLocation(hProgram, "MVP");
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(MVP));
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hIndexBuffer);
	glDrawElements(GL_POINTS, 3, GL_UNSIGNED_SHORT, 0);

	glutSwapBuffers();
}

void onTimerTick(int value) {
	yRotation += 5.0f;
	model = glm::rotate(mat4(), yRotation, vec3(0, 1, 0));

	glutTimerFunc(kTimerPeriod, onTimerTick, 0);
	glutPostRedisplay();
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
#ifdef __APPLE__
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
#else
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
#endif
	glutInitWindowSize(640, 480);
	glutCreateWindow("Animated character");	
	// Context created at this point
	glewExperimental = GL_TRUE; 
	GLenum glewRetVal = glewInit();
	if(!makeShaderProgram()) {
		cout << "Unable to build the shader program." << endl;
		return -1;
	}
	setUpModel();

	glutDisplayFunc(render);
	glutTimerFunc(kTimerPeriod, onTimerTick, 0);
	glutMainLoop();

	return 0;
}