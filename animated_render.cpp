#include <GL/glew.h>
#if __APPLE__
	#include <GLUT/glut.h>
#else 
	#include <GL/freeglut.h>
#endif
#include <algorithm>
#include <memory>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "MD5_MeshReader.h"
#include "MD5_AnimReader.h"
#include "Shader.h"

using std::unique_ptr;
using std::cout;
using std::endl;
using std::for_each;
using std::vector;

using glm::vec3;
using glm::vec4;
using glm::mat4;
using glm::quat;

// Structures / Classes
struct Vertex {
	vec3 position;
	vec4 indices;
	vec4 weights;
};

typedef vector<mat4> CurrentPose;

struct Mesh {
	vector<Vertex> vertices;
	vector<unsigned int> indices;

	vector<GLfloat> positions;
	GLuint hPositionBuffer;
	GLuint hIndexBuffer;
};

// GLOBALS
mat4 gModel;
mat4 gView;
mat4 gProjection;
unsigned int gCurrentFrame = 0;
MD5_AnimInfo gAnimInfo;
GLuint ghSkeletonPositionBuffer;

vector<Mesh> gMeshes;
CurrentPose gCurrentPose;

// Shaders
Shader *gpShader;
Shader *gpSkeletonShader;

// TEST
GLuint ghTestPositions;
GLuint ghTestIndices;

void computeCurrentPose() {
	vector<float> &frameData= gAnimInfo.framesData[gCurrentFrame];
	unsigned int numJoints = gAnimInfo.baseframeJoints.size();

	for(int i = 0; i < numJoints; ++i) {
		const BaseframeJoint &baseframeJoint = gAnimInfo.baseframeJoints[i];
		const JointInfo &jointInfo = gAnimInfo.jointsInfo[i];

		// Start with the default settings from basejoint
		vec3 position = baseframeJoint.position;
		quat orientation = baseframeJoint.orientation;

		// Start replacing with specific frame data
		int flags = jointInfo.flags;
		int offset = jointInfo.startIndex;

		if(flags & (1 << 0)) {
			position.x = frameData[offset++];
		}
		if(flags & (1 << 1)) {
			position.y = frameData[offset++];
		}
		if(flags & (1 << 2)) {
			position.z = frameData[offset++];
		}
		if(flags & (1 << 3)) {
			orientation.x = frameData[offset++];
		}
		if(flags & (1 << 4)) {
			orientation.y = frameData[offset++];
		}
		if(flags & (1 << 5)) {
			orientation.z = frameData[offset++];
		}

		// Compute the w-component of the quaternion
		float temp = 1 - orientation.x * orientation.x
					   - orientation.y * orientation.y
					   - orientation.z * orientation.z;
		if(temp < 0) {
			orientation.w = 0;
		} else {
			orientation.w = -1 * sqrtf(temp);
		}

		mat4 transM = glm::translate(mat4(), position);
		mat4 rotM = glm::mat4_cast(orientation);
		mat4 combinedM = transM * rotM;

		// Convert this point to model space
		if(jointInfo.parent > -1) {			
			mat4 parentM = gCurrentPose[jointInfo.parent];
			combinedM = parentM * combinedM;
		}

		gCurrentPose[i] = combinedM;
		cout << jointInfo.name << " " << combinedM[3][0] << ", " << combinedM[3][1] << ", " << combinedM[3][2] << endl;
	}
}

void initTestMesh() {
	float halfLen =5.0f;
	float zPos = -0.5f;

	GLfloat vertexData[] = {
		-halfLen,  halfLen, zPos, // V0
		-halfLen, -halfLen, zPos, // V1
		 halfLen, -halfLen, zPos, // V2
		 halfLen,  halfLen, zPos  // V3
	};

	GLushort indices[] = {0, 1, 2, 0, 2, 3};

	glGenBuffers(1, &ghTestPositions);
	glBindBuffer(GL_ARRAY_BUFFER, ghTestPositions);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData) * sizeof(GLfloat), vertexData, GL_STATIC_DRAW);

	glGenBuffers(1, &ghTestIndices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ghTestIndices); 
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices) * sizeof(GLushort), indices, GL_STATIC_DRAW);
}

void initModel() {
	MD5_MeshReader parser;
	MD5_MeshInfo meshInfo = parser.parse("Boblamp/boblampclean.md5mesh");

	for(auto meshIter = meshInfo.meshes.cbegin(); meshIter != meshInfo.meshes.cend(); ++meshIter) {
		const MD5_Mesh &md5mesh = *meshIter;
		Mesh mesh;

		cout << md5mesh.textureFilename << endl;
		int count = 0;

		for(auto vertIter = md5mesh.vertices.cbegin(); vertIter != md5mesh.vertices.cend(); ++vertIter) {
			const MD5_Vertex &md5vertex = *vertIter;
			vec3 finalPos(0);
			Vertex vert;
			vert.weights = vec4(0);
			vert.indices = vec4(0);

			for(int i = 0; i < md5vertex.weightCount; ++i) {
				const MD5_Weight &weight = md5mesh.weights[md5vertex.startWeight + i];
				const Joint &joint = meshInfo.joints[weight.jointIndex];

				// Convert joint-space position to model space position using.
				// TranslationMatrix * RotationMatrix * jointLocalPoint
				vec3 tempPos = tempPos = joint.orientation * weight.position + joint.position;				
				finalPos += tempPos * weight.weightBias;
				//finalPos = vec3(0);

				vert.indices[i] = weight.jointIndex;
				vert.weights[i] = weight.weightBias;
			}
			
			vert.position = finalPos;	
			//cout << "[" << count++ << "] " << vert.position.x << ", " << vert.position.y << ", " << vert.position.z << endl;
			mesh.vertices.push_back(vert);
			mesh.positions.push_back(vert.position.x);
			mesh.positions.push_back(vert.position.y);
			mesh.positions.push_back(vert.position.z);
		}

		gMeshes.push_back(mesh);
	};
}

void initAnimations() {
	MD5_AnimReader reader;
	gAnimInfo = reader.parse("Boblamp/boblampclean.md5anim");
	gCurrentPose.resize(gAnimInfo.baseframeJoints.size());
}

void initModelRenderData() {
	for(auto meshIter = gMeshes.begin(); meshIter != gMeshes.end(); ++meshIter) {
		Mesh &mesh = *meshIter;

		// positions
		glGenBuffers(1, &mesh.hPositionBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, mesh.hPositionBuffer);

		GLsizei bufferSize = mesh.positions.size() * sizeof(GLfloat);
		//cout << "Allocating a " << bufferSize << " byte buffer for vertex " << mesh.positions.size()/3 << " positions." << endl;
		glBufferData(GL_ARRAY_BUFFER, bufferSize, &mesh.positions[0], GL_STATIC_DRAW);
	}
}

void renderTestMesh() {
	mat4 MVP = gProjection * gView * gModel;

	glUseProgram(gpShader->handle());
	GLint location = glGetUniformLocation(gpShader->handle(), "MVP");
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(MVP));

	glBindBuffer(GL_ARRAY_BUFFER, ghTestPositions);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ghTestIndices);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}

void renderMeshes() {
	mat4 model = glm::rotate(mat4(), -90.0f, vec3(1.0, 0.0, 0.0));
	mat4 MVP = gProjection * gView * model;

	glUseProgram(gpShader->handle());
	GLint location = glGetUniformLocation(gpShader->handle(), "MVP");
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(MVP));

	for(auto meshIter = gMeshes.cbegin(); meshIter != gMeshes.cend(); ++meshIter) {
		const Mesh &mesh = *meshIter;
		glBindBuffer(GL_ARRAY_BUFFER, mesh.hPositionBuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glDrawArrays(GL_POINTS, 0, mesh.vertices.size());
	}
}

void renderSkeleton() {
	vector<GLfloat> vertices;

	for(int i = 0; i < gAnimInfo.jointsInfo.size(); ++i) {
		const JointInfo &jointInfo = gAnimInfo.jointsInfo[i];
		if(jointInfo.parent > 1) {
			mat4 transformM = gCurrentPose[i];
			mat4 parentTransformM = gCurrentPose[jointInfo.parent];
			
			vertices.push_back(transformM[3][0]);
			vertices.push_back(transformM[3][1]);
			vertices.push_back(transformM[3][2]);

			vertices.push_back(parentTransformM[3][0]);
			vertices.push_back(parentTransformM[3][1]);
			vertices.push_back(parentTransformM[3][2]);
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, ghSkeletonPositionBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), &vertices[0], GL_STATIC_DRAW);

	glUseProgram(gpSkeletonShader->handle());
	mat4 model = glm::rotate(mat4(), -90.0f, vec3(1.0, 0.0, 0.0));
	mat4 MVP = gProjection * gView * model;
	GLint location = glGetUniformLocation(gpShader->handle(), "MVP");
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(MVP));
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);


	glDrawArrays(GL_LINES, 0, 2 * vertices.size());
}

void render() {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPointSize(5.0f);

	renderTestMesh();
	renderMeshes();
	renderSkeleton();

	glutSwapBuffers();
}

void initGL(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitWindowSize(600, 600);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	//glutInitContextVersion(2, 0);
	glutCreateWindow("Baseframe Boogaloo");
	
	if(glewInit() != GLEW_OK) {
		cout << "Could not initialize GLEW." << endl;
	}
	
	const GLubyte *version = glGetString(GL_VERSION);
	const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

	cout << version << endl;
	cout << "GLSL version: " << glslVersion << endl;

	glutDisplayFunc(render);
}

void initShader() {
	gpShader = new Shader();
	
	if(!gpShader->compile("baseframe_shader.vert", GL_VERTEX_SHADER)) {
		cout << "Could not build baseframe_shader.vert" << endl;
		exit(EXIT_FAILURE);
	}

	glBindAttribLocation(gpShader->handle(), 0, "VertexPosition");

	if(!gpShader->compile("baseframe_shader.frag", GL_FRAGMENT_SHADER)) {
		cout << "Could not build baseframe_shader.frag" << endl;
		exit(EXIT_FAILURE);
	}

	if(!gpShader->link()) {
		cout << "Could not link the shader." << endl;
		exit(EXIT_FAILURE);
	}
}

void initSkeletonShader() {
	gpSkeletonShader = new Shader();
	
	if(!gpSkeletonShader->compile("Skeleton.vert", GL_VERTEX_SHADER)) {
		cout << "Problem compiling the skeleton.vert shader." << endl;
		exit(EXIT_FAILURE);
	}

	if(!gpSkeletonShader->compile("Skeleton.frag", GL_FRAGMENT_SHADER)) {
		cout << "Problem compiling the skeleton.frag shader." << endl;
		exit(EXIT_FAILURE);
	}

	glBindAttribLocation(gpSkeletonShader->handle(), 0, "VertexPosition");

	if(!gpSkeletonShader->link()) {
		cout << "Could not link the skeleton shader." << endl;
		exit(EXIT_FAILURE);
	}

	// We'll do this here for now
	glGenBuffers(1, &ghSkeletonPositionBuffer);
}

void initCamera() {
	// The MD5 format points the model along the z-axis headfirst, so we need to rotate the model.
	//gModel = glm::rotate(mat4(), -90.0f, vec3(1.0, 0.0, 0.0));
	//gView = glm::translate(mat4(), vec3(0, -20, -120));
	gView = glm::translate(mat4(), vec3(0, -20, -150));
	gProjection = glm::perspective(45.0f, 1.0f, 0.1f, 1000.0f);
}

int main(int argc, char **argv) {	
	initGL(argc, argv);
	initShader();
	initSkeletonShader();
	initCamera();
	initTestMesh();
	initModel();
	initAnimations();
	initModelRenderData();

	// Temp
	computeCurrentPose();

	glutMainLoop();

	return 0;
}