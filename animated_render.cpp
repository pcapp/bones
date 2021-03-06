#include <GL/glew.h>
#if __APPLE__
	#include <GLUT/glut.h>
#else 
	#include <GL/freeglut.h>
#endif
#include <algorithm>
#include <memory>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <utility>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SOIL.h>

#include "MD5_MeshReader.h"
#include "MD5_AnimReader.h"
#include "Shader.h"

#define MAX_JOINTS 64

using std::map;
using std::unique_ptr;
using std::cout;
using std::endl;
using std::make_pair;
using std::for_each;
using std::string;
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
	vector<GLushort> indices;
	
	// Used to calculate VBO data
	vector<GLfloat> positions;
	vector<int> jointIndices;
	vector<GLfloat> jointWeights;
	vector<GLfloat> textureCoords;

	GLuint hPositionBuffer;
	GLuint hJointIndexBuffer;
	GLuint hJointWeightBuffer;
	GLuint hIndexBuffer;
	GLuint hTextureCoordsBuffer;
	GLuint texID;
};

// GLOBALS
map<string, GLint> gNameToTexID;

mat4 gModel;
mat4 gView;
mat4 gProjection;

vector<mat4> gIBPMatrices;
vector<mat4> gMatrixPalette;

unsigned int gCurrentFrame = 0;
MD5_AnimInfo gAnimInfo;
GLuint ghSkeletonPositionBuffer;

vector<Mesh> gMeshes;
CurrentPose gCurrentPose;

// Shaders
Shader *gpShader;
Shader *gpSkeletonShader;
Shader *gpTestMeshShader;

// Debug Rendering
GLuint ghTestPositions;
GLuint ghTestTextureCoords;
GLuint ghTestIndices;
GLuint ghTexID;

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
		//cout << jointInfo.name << " " << combinedM[3][0] << ", " << combinedM[3][1] << ", " << combinedM[3][2] << endl;
	}
}

void initTestMesh() {
	float halfLen = 25.0f;
	float zPos = -0.5f;

	GLfloat vertexData[] = {
		-halfLen,  halfLen, zPos, // V0
		-halfLen, -halfLen, zPos, // V1
		 halfLen, -halfLen, zPos, // V2
		 halfLen,  halfLen, zPos  // V3
	};

	GLfloat uvData[] = {
		0.0f, 1.0f, // V0
		0.0f, 0.0f, // V1
		1.0f, 0.0f, // V2
		1.0f, 1.0f  // V3
	};

	GLushort indices[] = {0, 1, 2, 0, 2, 3};

	// position
	glGenBuffers(1, &ghTestPositions);
	glBindBuffer(GL_ARRAY_BUFFER, ghTestPositions);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData) * sizeof(GLfloat), vertexData, GL_STATIC_DRAW);

	// texture coordinates
	glGenBuffers(1, &ghTestTextureCoords);
	glBindBuffer(GL_ARRAY_BUFFER, ghTestTextureCoords);
	glBufferData(GL_ARRAY_BUFFER, sizeof(uvData) * sizeof(GLfloat), uvData, GL_STATIC_DRAW);

	// triangle indices
	glGenBuffers(1, &ghTestIndices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ghTestIndices); 
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices) * sizeof(GLushort), indices, GL_STATIC_DRAW);
}

void initModel() {
	MD5_MeshReader parser;
	MD5_MeshInfo meshInfo = parser.parse("Boblamp/boblampclean.md5mesh");

	// Process each mesh found in the md5mesh file
	for(auto meshIter = meshInfo.meshes.cbegin(); meshIter != meshInfo.meshes.cend(); ++meshIter) {
		const MD5_Mesh &md5mesh = *meshIter;
		Mesh mesh;

		// Prepare the vertices
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

				vert.indices[i] = weight.jointIndex;
				vert.weights[i] = weight.weightBias;

				mesh.jointIndices.push_back(weight.jointIndex);
				mesh.jointWeights.push_back(weight.weightBias);
				//mesh.jointWeights.push_back(0);
			}
			
			// WE NEED 4 ENTRIES; RIGHT NOW I ONLY HAVE WEIGHTCOUNT ENTRIES
			for(int i = md5vertex.weightCount; i < 4; ++i) {
				mesh.jointIndices.push_back(-1);
				mesh.jointWeights.push_back(0.0f);
			}

			vert.position = finalPos;	
			
			mesh.vertices.push_back(vert);
			mesh.positions.push_back(vert.position.x);
			mesh.positions.push_back(vert.position.y);
			mesh.positions.push_back(vert.position.z);

			mesh.textureCoords.push_back(md5vertex.u);
			mesh.textureCoords.push_back(md5vertex.v);
		}
		
		// Set up the triangle indices
		for(int i = 0; i < md5mesh.triangles.size(); ++i) {
			const MD5_Triangle &triangle = md5mesh.triangles[i];
			mesh.indices.push_back(triangle.indices[0]);
			mesh.indices.push_back(triangle.indices[1]);
			mesh.indices.push_back(triangle.indices[2]);
		}

		// Prepare the texutre, if necessary
		GLuint texID = 0;

		if(gNameToTexID.find(md5mesh.textureFilename) == gNameToTexID.end()) {

			// TODO: Prepare this in a neutral manner, i.e. not for just Boblamp
			string fullname = "Boblamp/" + md5mesh.textureFilename;
			texID = SOIL_load_OGL_texture(fullname.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, 0);
			if(texID == 0) {
				cout << "Error preparing " << fullname << " as a texture." << endl;
				cout << SOIL_last_result() << endl;
			} else {
				gNameToTexID.insert(make_pair(md5mesh.textureFilename, texID));
			}
		}

		mesh.texID = texID;

		gMeshes.push_back(mesh);
	};

	// Set up the inverse bind pose matrix for each joint
	for(int i = 0; i < meshInfo.joints.size(); ++i) {
		const Joint &joint = meshInfo.joints[i];

		// rotM and transM are model space transformation matrices. 
		// Combine then invert them to construct the inverse bind pose matrix for each joint.
		mat4 rotM = glm::mat4_cast(joint.orientation);
		mat4 transM = glm::translate(mat4(), joint.position);
		mat4 combinedM = transM * rotM;
		mat4 InvBindPosM = glm::inverse(combinedM);
		gIBPMatrices.push_back(InvBindPosM);
	}

	// We can also initialize the skinning palette to have the required number of joints.
	gMatrixPalette.resize(meshInfo.joints.size());
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
		glBufferData(GL_ARRAY_BUFFER, bufferSize, &mesh.positions[0], GL_STATIC_DRAW);

		// joint indices
		glGenBuffers(1, &mesh.hJointIndexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, mesh.hJointIndexBuffer);
		bufferSize = mesh.jointIndices.size() * sizeof(int);
		glBufferData(GL_ARRAY_BUFFER, bufferSize, &mesh.jointIndices[0], GL_STATIC_DRAW);

		// joint weights
		glGenBuffers(1, &mesh.hJointWeightBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, mesh.hJointWeightBuffer);
		bufferSize = mesh.jointWeights.size() * sizeof(GLfloat);
		glBufferData(GL_ARRAY_BUFFER, bufferSize, &mesh.jointWeights[0], GL_STATIC_DRAW);

		// texture coordinates
		glGenBuffers(1, &mesh.hTextureCoordsBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, mesh.hTextureCoordsBuffer);
		bufferSize = mesh.textureCoords.size() * sizeof(GLfloat);
		glBufferData(GL_ARRAY_BUFFER, bufferSize, &mesh.textureCoords[0], GL_STATIC_DRAW);
		
		// triangle indices
		glGenBuffers(1, &mesh.hIndexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.hIndexBuffer);
		bufferSize = mesh.indices.size() * sizeof(GLushort);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufferSize, &mesh.indices[0], GL_STATIC_DRAW);
	}
}

void renderTestMesh() {
	mat4 MVP = gProjection * gView * gModel;

	glUseProgram(gpTestMeshShader->handle());
	GLint location = glGetUniformLocation(gpTestMeshShader->handle(), "MVP");
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(MVP));

	// positions
	glBindBuffer(GL_ARRAY_BUFFER, ghTestPositions);
	GLint positionLoc = glGetAttribLocation(gpTestMeshShader->handle(), "VertexPosition");
	glEnableVertexAttribArray(positionLoc);
	glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// texture coordinates
	glBindBuffer(GL_ARRAY_BUFFER, ghTestTextureCoords);
	GLint textureCoordLoc = glGetAttribLocation(gpTestMeshShader->handle(), "TextureCoords");
	glEnableVertexAttribArray(textureCoordLoc);
	glVertexAttribPointer(textureCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);

	// texture 
	GLint textureLoc = glGetUniformLocation(gpTestMeshShader->handle(), "tex0");
	glUniform1d(textureLoc, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ghTexID);

	// triangle indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ghTestIndices);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}

void renderMeshes() {
	mat4 model = glm::rotate(mat4(), -90.0f, vec3(1.0, 0.0, 0.0));
	mat4 MVP = gProjection * gView * model;

	glUseProgram(gpShader->handle());
	GLint location = glGetUniformLocation(gpShader->handle(), "MVP");
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(MVP));

	std::stringstream name;

	for(int i = 0; i < gMatrixPalette.size(); ++i) {
		name.str("");
		name.clear();
		name << "MatrixPalette[" << i << "]";
		location = glGetUniformLocation(gpShader->handle(), name.str().c_str());
		if(location == -1) {
			cout << name.str() << " did not correspond to a valid uniform location." << endl;
		} else {
			glUniformMatrix4fv(location, 1, false, glm::value_ptr(gMatrixPalette[i]));
		}
	}

	for(auto meshIter = gMeshes.cbegin(); meshIter != gMeshes.cend(); ++meshIter) {
		const Mesh &mesh = *meshIter;

		// baseframe position
		glBindBuffer(GL_ARRAY_BUFFER, mesh.hPositionBuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		// joint indices
		glBindBuffer(GL_ARRAY_BUFFER, mesh.hJointIndexBuffer);
		glVertexAttribPointer(1, 4, GL_INT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		// joint weights
		glBindBuffer(GL_ARRAY_BUFFER, mesh.hJointWeightBuffer);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(2);

		// texture coordinates
		GLint textureCoordsLoc = glGetAttribLocation(gpShader->handle(), "TextureCoords");
		if(textureCoordsLoc == -1) {
			cout << "Error: Cannot get the location of TextureCoords attribute." << endl;
		}

		GLint imageTexLoc = glGetUniformLocation(gpShader->handle(), "imageTex");
		if(imageTexLoc == -1) {
			cout << "Error: Cannot get the location of the imageTex uniform." << endl;
		} else {
			glUniform1d(imageTexLoc, 0);
		}

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mesh.texID);

		glBindBuffer(GL_ARRAY_BUFFER, mesh.hTextureCoordsBuffer);
		glVertexAttribPointer(textureCoordsLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(textureCoordsLoc);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.hIndexBuffer);
		glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_SHORT, 0);
		//glDrawArrays(GL_POINTS, 0, mesh.vertices.size());
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
	GLint location = glGetUniformLocation(gpSkeletonShader->handle(), "MVP");
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(MVP));
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);


	glDrawArrays(GL_LINES, 0, 2 * vertices.size());
}

void updateMatrixPalette() {
	const int numJoints = gCurrentPose.size();

	for(int i = 0 ; i < numJoints; ++i) {
		mat4 &IBP = gIBPMatrices[i];
		mat4 &C_j_to_m = gCurrentPose[i];

		gMatrixPalette[i] = C_j_to_m * IBP;
	}
}

void render() {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPointSize(5.0f);

	//renderTestMesh();
	//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	glEnable(GL_DEPTH_TEST);
	renderMeshes();
	//glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	//glDisable(GL_DEPTH_TEST);
	//renderSkeleton();

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

	glEnable(GL_DEPTH_TEST);
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
	glBindAttribLocation(gpShader->handle(), 1, "JointIndices");
	glBindAttribLocation(gpShader->handle(), 2, "JointWeights");

	if(!gpShader->compile("baseframe_shader.frag", GL_FRAGMENT_SHADER)) {
		cout << "Could not build baseframe_shader.frag" << endl;
		exit(EXIT_FAILURE);
	}

	if(!gpShader->link()) {
		cout << "Could not link the shader." << endl;
		exit(EXIT_FAILURE);
	}
}

void initTestMeshShader() {
	gpTestMeshShader = new Shader();
	const string &vertShaderName = "testmesh.vert";
	const string &fragShaderName = "testmesh.frag";

	if(!gpTestMeshShader->compile(vertShaderName, GL_VERTEX_SHADER)) {
		cout << "Could not build " << vertShaderName << endl;
		exit(EXIT_FAILURE);
	}

	glBindAttribLocation(gpTestMeshShader->handle(), 0, "VertexPosition");
	glBindAttribLocation(gpTestMeshShader->handle(), 1, "TextureCoords");
	
	if(!gpTestMeshShader->compile(fragShaderName, GL_FRAGMENT_SHADER)) {
		cout << "Could not build " << fragShaderName << endl;
		exit(EXIT_FAILURE);
	}

	if(!gpTestMeshShader->link()) {
		cout << "Could not link the shader." << endl;
		exit(EXIT_FAILURE);
	}

	// Set up the texture down here
	ghTexID = SOIL_load_OGL_texture("UV_mapper.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y);
	if(ghTexID == 0) {
		cout << "Could not load the UV_mapper.jpg file and make a GL texture out of it." << endl;
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
	initTestMeshShader();
	initModel();
	initAnimations();
	initModelRenderData();

	// Temp
	computeCurrentPose();
	updateMatrixPalette();

	glutMainLoop();

	return 0;
}