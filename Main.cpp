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

struct Bounds {
	float minX;
	float maxX;
	float minY;
	float maxY;
	float minZ;
	float maxZ;
};

////////////////////////
// GLOBALS
////////////////////////
AnimInfo g_AnimInfo;

GLuint hProgram;
GLuint hVertShader;
GLuint hFragShader;
GLuint vao;
GLuint hIndexBuffer;

float yRotation = 0.0f;

mat4 model;
mat4 view;
mat4 projection;
mat4 MVP;

Bounds bounds;

const int kTimerPeriod = 50;
const float kFovY = 45.0f;

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

void CalculateBounds(GLfloat *vertices, int n, Bounds &bounds) {
	if(n % 3 != 0) {		
		throw runtime_error("Each vertex must have 3 floats");
	}

	bounds.minX = bounds.minY = bounds.minZ = std::numeric_limits<GLfloat>::max();
	bounds.maxX = bounds.maxY = bounds.maxZ = std::numeric_limits<GLfloat>::min();

	for(int i = 0; i < n; /* Incremented in loop */) {
		GLfloat x = vertices[i++];
		GLfloat y = vertices[i++];
		GLfloat z = vertices[i++];

		cout << (i/3) << " " << x << " " << y << " " << z << endl;

		if(x < bounds.minX) {
			bounds.minX = x;
		}
		if(x > bounds.maxX) {
			bounds.maxX = x;
		}
		if(y < bounds.minY) {
			bounds.minY = y;
		}
		if(y > bounds.maxY) {
			bounds.maxY = y;
		}
		if(z < bounds.minZ) {
			bounds.minZ = z;
		}
		if(z > bounds.maxZ) {
			bounds.maxZ = z;
		}
	}
}

void setUpModel() {
	float zPos = 0.0f;
	vector<GLfloat> vertices;
	vector<GLfloat> colors;

	for(int i = 0; i < g_AnimInfo.skeleton.joints.size(); ++i) {
		Joint &joint = g_AnimInfo.skeleton.joints[i];
		mat4 &jointToWorld = joint.jointToWorld;
		/*GLfloat x = jointToWorld[3][0];
		GLfloat y = jointToWorld[3][1];
		GLfloat z = jointToWorld[3][2];*/
		GLfloat x = joint.position.x;
		GLfloat y = joint.position.y;
		GLfloat z = joint.position.z;

		vertices.push_back(x);
		vertices.push_back(y);
		vertices.push_back(z);

		colors.push_back(1.0f);
		colors.push_back(0.0f);
		colors.push_back(0.0f);
		colors.push_back(1.0f);
	}
	/*vertices.push_back(-0.75f);
	vertices.push_back(0.0f);
	vertices.push_back(zPos);
	vertices.push_back(0.75f);
	vertices.push_back(0.0f);
	vertices.push_back(zPos);
	vertices.push_back(0.0f);
	vertices.push_back(0.75f);
	vertices.push_back(zPos);

	
	colors.push_back(1.0f);
	colors.push_back(0.0f);
	colors.push_back(0.0f);
	colors.push_back(1.0f);

	colors.push_back(0.0f);
	colors.push_back(1.0f);
	colors.push_back(0.0f);
	colors.push_back(1.0f);

	colors.push_back(0.0f);
	colors.push_back(0.0f);
	colors.push_back(1.0f);
	colors.push_back(1.0f);*/

	CalculateBounds(&vertices[0], vertices.size(), bounds);
	cout << "Model bounds: " << endl;
	cout << "X: " << bounds.minX << " to " << bounds.maxX << endl;
	cout << "Y: " << bounds.minY << " to " << bounds.maxY << endl;
	cout << "Z: " << bounds.minZ << " to " << bounds.maxZ << endl;

	// Set up the vertex buffer object
	GLuint hVerticesBuffer;
	GLuint hColorsBuffer;

	glGenBuffers(1, &hVerticesBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, hVerticesBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), &vertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &hColorsBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, hColorsBuffer);
	glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GLfloat), &colors[0], GL_STATIC_DRAW);

	// Set up the vertex array object
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glEnableVertexAttribArray(0); // Vertex position
	glEnableVertexAttribArray(1); // RGBA
	
	glBindBuffer(GL_ARRAY_BUFFER, hVerticesBuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, hColorsBuffer);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Set up the indices
	//GLushort indices[3] = {0, 1, 2};
	vector<GLushort> indices;
	//indices.push_back(0);
	//indices.push_back(1);
	//indices.push_back(2);
	///*

	for(int i = 0; i < vertices.size() / 3; ++i) {
		indices.push_back(i);
	}
	/*
	for(int i = 0; i < g_AnimInfo.skeleton.joints.size(); ++i) {
		indices.push_back(i);
	}*/

	glGenBuffers(1, &hIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), &indices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void setUpCamera() {
	// Set up the camera to frame the model. 
	projection = glm::perspective(kFovY, 4.0f/3.0f, 0.1f, 100000.0f);
	
	float modelLen = max(bounds.minY, bounds.maxY);
	float totalLen = modelLen / 0.8f;
	float half_fov = kFovY / 2.0f; // Normally done by our camera

	float radians = half_fov * 3.1459895f / 180.0f;
	float z = totalLen / tan(radians);
	
	cout << "Moving the camera to: " << z << endl;
	view = glm::translate(mat4(), vec3(0, 0, -z));
	//view = glm::translate(mat4(), vec3(0, 0, -5000));

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
	glDrawElements(GL_POINTS, g_AnimInfo.skeleton.joints.size(), GL_UNSIGNED_SHORT, 0);

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
	setUpCamera();

	glutDisplayFunc(render);
	glutTimerFunc(kTimerPeriod, onTimerTick, 0);
	glutMainLoop();

	return 0;
}