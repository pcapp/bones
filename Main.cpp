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
#include <glm/gtc/quaternion.hpp>

#include "AnimCore.h"
#include "MD5Reader.h"

using namespace std;
using glm::mat4;
using glm::vec3;
using glm::vec4;
using glm::quat;

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
MD5_AnimInfo g_AnimInfo;

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

vector<int> lineIndices;
GLuint hSkeletonBuffer;
GLuint hSkeletonVAO;
GLuint numBonesToDraw;

Bounds bounds;

int curFrame = 75;
int elapsedTime = 0;

const int kTimerPeriod = 50;
const float kFovY = 45.0f;

struct FrameJoint {
	string name;
	vec3 position;
	quat orientation;
	int parentIndex;
};

vector<vector<FrameJoint>> frameSkeletons;

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

ostream &operator<<(ostream &out, const vec4 &v) {
	out << "<" << v.x << ", " << v.y << ", " << v.z << ">";
	return out;
}

void createFrameSkeletons() {
	const int kNumJoints = g_AnimInfo.baseframeJoints.size();
	
	for(int i = 0; i < g_AnimInfo.numFrames; ++i) {
		vector<float> frameData = g_AnimInfo.framesData[i]; // Render frame 0
		vector<FrameJoint> frameSkeleton;

		for(int i = 0; i < kNumJoints; ++i) {
			FrameJoint frameJoint;
			const BaseframeJoint &baseframeJoint = g_AnimInfo.baseframeJoints[i];
			const JointInfo &jointInfo = g_AnimInfo.jointsInfo[i];


			// Start with the default settings from basejoint
			frameJoint.position = baseframeJoint.position;
			frameJoint.orientation = baseframeJoint.orientation;
			frameJoint.name = jointInfo.name;
			frameJoint.parentIndex = jointInfo.parent;

			// Start replacing with specific frame data
			int flags = jointInfo.flags;
			int offset = jointInfo.startIndex;

			if(flags & (1 << 0)) {
				frameJoint.position.x = frameData[offset++];
			}
			if(flags & (1 << 1)) {
				frameJoint.position.y = frameData[offset++];
			}
			if(flags & (1 << 2)) {
				frameJoint.position.z = frameData[offset++];
			}
			if(flags & (1 << 3)) {
				frameJoint.orientation.x = frameData[offset++];
			}
			if(flags & (1 << 4)) {
				frameJoint.orientation.y = frameData[offset++];
			}
			if(flags & (1 << 5)) {
				frameJoint.orientation.z = frameData[offset++];
			}

			// Compute the w-component of the quaternion
			float temp = 1 - frameJoint.orientation.x * frameJoint.orientation.x
						   - frameJoint.orientation.y * frameJoint.orientation.y
						   - frameJoint.orientation.z * frameJoint.orientation.z;
			if(temp < 0) {
				frameJoint.orientation.w = 0;
			} else {
				frameJoint.orientation.w = -1 * sqrtf(temp);
			}

			// Convert this point to model space
			if(jointInfo.parent > -1) {			
				const FrameJoint &parent = frameSkeleton[frameJoint.parentIndex];

				vec3 rotPos = parent.orientation * frameJoint.position;
				frameJoint.position.x = rotPos.x + parent.position.x;
				frameJoint.position.y = rotPos.y + parent.position.y;
				frameJoint.position.z = rotPos.z + parent.position.z;

				frameJoint.orientation = parent.orientation * frameJoint.orientation;
				glm::normalize(frameJoint.orientation);
			}

			frameSkeleton.push_back(frameJoint);
		}

		frameSkeletons.push_back(frameSkeleton);
	}
}

void setUpModel() {
	float zPos = 0.0f;
	vector<GLfloat> vertices;
	vector<GLfloat> colors;

	vector<FrameJoint> frameSkeleton = frameSkeletons[curFrame];

	for(int i = 0; i < frameSkeleton.size(); ++i) {
		const FrameJoint &joint = frameSkeleton[i];

		vertices.push_back(joint.position.x);
		vertices.push_back(joint.position.y);
		vertices.push_back(joint.position.z);

		colors.push_back(1.0f);
		colors.push_back(0.0f);
		colors.push_back(0.0f);
		colors.push_back(1.0f);
	}
	
	CalculateBounds(&vertices[0], vertices.size(), bounds);

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
	vector<GLushort> indices;
	for(int i = 0; i < vertices.size() / 3; ++i) {
		indices.push_back(i);
	}
	
	glGenBuffers(1, &hIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), &indices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void setUpSkeletonRendering() {
	vector<GLfloat> vertexData;
	numBonesToDraw = 0;

	vector<FrameJoint> frameSkeleton = frameSkeletons[curFrame];

	for(int i = 0; i < frameSkeleton.size(); ++i) {
		FrameJoint &joint = frameSkeleton[i];

		if(joint.parentIndex > -1) {
			FrameJoint &parentJoint = frameSkeleton[joint.parentIndex];

			vertexData.push_back(joint.position.x);
			vertexData.push_back(joint.position.y);
			vertexData.push_back(joint.position.z);
			vertexData.push_back(0.0f); // R
			vertexData.push_back(1.0f); // G
			vertexData.push_back(0.0f); // B
			vertexData.push_back(1.0f); // A

			// End joint			
			vertexData.push_back(parentJoint.position.x);
			vertexData.push_back(parentJoint.position.y);
			vertexData.push_back(parentJoint.position.z);
			vertexData.push_back(0.0f); // R
			vertexData.push_back(1.0f); // G
			vertexData.push_back(0.0f); // B
			vertexData.push_back(1.0f); // A

			++numBonesToDraw;
		}
	}

	glGenBuffers(1, &hSkeletonBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, hSkeletonBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(GLfloat), &vertexData[0], GL_STATIC_DRAW);

	// Create the VAO
	glGenVertexArrays(1, &hSkeletonVAO);
	glBindVertexArray(hSkeletonVAO);

	glEnableVertexAttribArray(0); // VertexPosition
	glEnableVertexAttribArray(1); // VertexRGBA

	GLsizei stride = 7 * sizeof(GLfloat);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (const GLvoid *)(3 * sizeof(GLfloat)));

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void setUpCamera() {
	// Set up the camera to frame the model. 
	projection = glm::perspective(kFovY, 4.0f/3.0f, 0.1f, 1000.0f);
	
	float modelLen = max(bounds.minY, bounds.maxY);
	float totalLen = modelLen / 0.8f;
	float half_fov = kFovY / 2.0f; // Normally done by our camera

	float radians = half_fov * 3.1459895f / 180.0f;
	float z = totalLen / tan(radians);
	
	//cout << "Moving the camera to: " << z << endl;
	//view = glm::translate(mat4(), vec3(0, 0, -z));
	view = glm::translate(mat4(), vec3(0, -20, -120));
	
	// temp
	model = glm::rotate(mat4(), -90.0f, vec3(1.0, 0.0, 0.0));

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
	glDrawElements(GL_POINTS, g_AnimInfo.baseframeJoints.size(), GL_UNSIGNED_SHORT, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindVertexArray(hSkeletonVAO);
	glBindBuffer(GL_ARRAY_BUFFER, hSkeletonBuffer);
	glDrawArrays(GL_LINES, 0, numBonesToDraw * 2);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

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
	const string meshFilename("Boblamp/boblampclean.md5mesh");
	const string animFilename("Boblamp/boblampclean.md5anim");

	try {
		g_AnimInfo = reader.parse(meshFilename, animFilename);
	}
	catch(exception &e) {
		cout << e.what() << endl;
		return -1;
	}
	
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

	createFrameSkeletons();
	setUpModel();
	setUpSkeletonRendering();
	setUpCamera();

	glutDisplayFunc(render);
	//glutTimerFunc(kTimerPeriod, onTimerTick, 0);
	glutMainLoop();

	return 0;
}