 #include <GL/glew.h>
#if __APPLE__
	#include <GLUT/glut.h>
#else 
	#include <GL/glut.h>
#endif

#include <algorithm>
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
MD5_VO g_MD5_VO;

GLuint hSimpleProgram;
GLuint hVertShader;
GLuint hFragShader;
GLuint vao;
GLuint hIndexBuffer;

GLuint hMeshProgram;
//GLuint hMeshVBO;
//GLuint hMeshVAO;
//GLuint hMeshIndexBuffer;

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

bool frameChanged = true;
int curFrame = 0;
int elapsedTime = 0;
GLuint hVerticesBuffer;
GLuint hColorsBuffer;

const int kTimerPeriod = 50;
const float kFovY = 45.0f;

struct FrameJoint {
	string name;
	vec3 position;
	quat orientation;
	int parentIndex;
};

vector<vector<FrameJoint>> frameSkeletons;

struct RenderableMesh {
	MD5_Mesh mesh;
	GLuint hVBO;
	GLuint hVAO;
	GLuint hIndexBuffer;
};

vector<RenderableMesh> g_Meshes;

bool buildShader(GLuint &hProgram, GLuint &hShader, const char *filename, GLuint shaderType) {
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
	hSimpleProgram = glCreateProgram();

	if(!buildShader(hSimpleProgram, hVertShader, "simple.vert", GL_VERTEX_SHADER)) {		
		return false;
	}

	glBindAttribLocation(hSimpleProgram, 0, "VertexPosition");
	glBindAttribLocation(hSimpleProgram, 1, "VertexRGBA");

	if(!buildShader(hSimpleProgram, hFragShader, "simple.frag", GL_FRAGMENT_SHADER)) {
		return false;
	}

	GLint linkStatus;
	glLinkProgram(hSimpleProgram);
	glGetProgramiv(hSimpleProgram, GL_LINK_STATUS, &linkStatus);
	if(GL_FALSE == linkStatus) {
		cout << "Error linking the shader program." << endl;
		GLint logLength;
		glGetProgramiv(hSimpleProgram, GL_INFO_LOG_LENGTH, &logLength);

		if(logLength > 0) {
			GLsizei bytesRead;
			char *log = new char[logLength];
			glGetProgramInfoLog(hSimpleProgram, logLength, &bytesRead, log);	
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
	const int kNumJoints = g_MD5_VO.animations[0].baseframeJoints.size();
	
	for(int i = 0; i < g_MD5_VO.animations[0].numFrames; ++i) {
		vector<float> frameData = g_MD5_VO.animations[0].framesData[i]; // Render frame 0
		vector<FrameJoint> frameSkeleton;

		for(int i = 0; i < kNumJoints; ++i) {
			FrameJoint frameJoint;
			const BaseframeJoint &baseframeJoint = g_MD5_VO.animations[0].baseframeJoints[i];
			const JointInfo &jointInfo = g_MD5_VO.animations[0].jointsInfo[i];


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

void updateJointData() {
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
	
	glBindBuffer(GL_ARRAY_BUFFER, hVerticesBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), &vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, hColorsBuffer);
	glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GLfloat), &colors[0], GL_STATIC_DRAW);
}

void updateSkeletonData() {
	vector<GLfloat> vertexData;
	numBonesToDraw = 0;

	vector<FrameJoint> &frameSkeleton = frameSkeletons[curFrame];

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

	glBindBuffer(GL_ARRAY_BUFFER, hSkeletonBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(GLfloat), &vertexData[0], GL_STATIC_DRAW);
}

void setUpMeshRendering() {
	unsigned int count = 0;
	for(auto &mesh : g_MD5_VO.mesh.meshes) {
		cout << "Mesh [" << count++ << "] " << mesh.textureFilename << endl;
		RenderableMesh renderMesh;
		renderMesh.mesh = mesh;

		// Set up the vertex positions for the mesh
		GLuint hVBO = 0;
		glGenBuffers(1, &hVBO);
		GLsizei bufferSize = mesh.vertices.size() * sizeof(float);
		
		vector<float> positions(mesh.vertices.size(), 0);

		glBindBuffer(GL_ARRAY_BUFFER, hVBO);
		glBufferData(GL_ARRAY_BUFFER, bufferSize, &positions[0], GL_STATIC_DRAW);
		renderMesh.hVBO = hVBO;

		// Set up the indices
		GLuint hIndexBuffer;
		glGenBuffers(1, &hIndexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hIndexBuffer);
		
		vector<unsigned short> indices;
		unsigned count = 0;
		for_each(mesh.triangles.cbegin(), mesh.triangles.cend(), [&](const MD5_Triangle &tri) {
			//cout << "[" << count++ << "] " << tri.indices[0] << ", " << tri.indices[1] << ", " << tri.indices[2] << endl;
			indices.push_back(tri.indices[0]);
			indices.push_back(tri.indices[1]);
			indices.push_back(tri.indices[2]);
		});

		bufferSize = indices.size() * sizeof(unsigned short);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufferSize, &indices[0], GL_STATIC_DRAW);

		renderMesh.hIndexBuffer = hIndexBuffer;

		// Set up the VAO
		glGenVertexArrays(1, &renderMesh.hVAO);
		glBindVertexArray(renderMesh.hVAO);

		glEnableVertexAttribArray(0); // VertexPosition
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glBindVertexArray(0);

		g_Meshes.push_back(renderMesh);
	}
}

void setUpMeshShader() {
	hMeshProgram = glCreateProgram();

	if(GL_FALSE == hMeshProgram) {
		cout << "Could not create a shader program handle." << endl;
		return;
	}

	GLuint hVertShader;
	GLuint hFragShader;

	if(!buildShader(hMeshProgram, hVertShader, "mesh.vert", GL_VERTEX_SHADER)) {
		cout << "Could not build mesh.vert." << endl;
		return;
	}

	glBindAttribLocation(hMeshProgram, 0, "VertexPosition");

	if(!buildShader(hMeshProgram, hFragShader, "mesh.frag", GL_FRAGMENT_SHADER)) {
		cout << "Could not compile mesh.frag" << endl;
		return;
	}

	GLint linkStatus;
	glLinkProgram(hMeshProgram);
	glGetProgramiv(hMeshProgram, GL_LINK_STATUS, &linkStatus);
	if(GL_FALSE == linkStatus) {
		cout << "Error linking the mesh shader program." << endl;
		GLint logLength;
		glGetProgramiv(hMeshProgram, GL_INFO_LOG_LENGTH, &logLength);

		if(logLength > 0) {
			GLsizei bytesRead;
			char *log = new char[logLength];
			glGetProgramInfoLog(hMeshProgram, logLength, &bytesRead, log);	
			cout << log << endl;	
			delete []log;
		}
	}
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

void renderSkeleton() {
	if(frameChanged) {
		updateJointData();
		updateSkeletonData();
		frameChanged = false;
	}

	glUseProgram(hSimpleProgram);
	MVP = projection * view * model;
	GLint location = glGetUniformLocation(hSimpleProgram, "MVP");
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(MVP));
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hIndexBuffer);
	glDrawElements(GL_POINTS, g_MD5_VO.animations[0].baseframeJoints.size(), GL_UNSIGNED_SHORT, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindVertexArray(hSkeletonVAO);
	glBindBuffer(GL_ARRAY_BUFFER, hSkeletonBuffer);
	glDrawArrays(GL_LINES, 0, numBonesToDraw * 2);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void updateVertexPositions(RenderableMesh &renderMesh) {
	MD5_Mesh &mesh = renderMesh.mesh;

	// We can use an interpolated version later
	vector<FrameJoint> &curSkeleton = frameSkeletons[curFrame];
	vector<GLfloat> updatedBuffer;

	for(MD5_Vertex &vertex: mesh.vertices) {
		vec3 pos(0,0,0);

		for(int i = 0; i < vertex.weightCount; ++i) {
			// Get the weight
			const MD5_Weight &weight = mesh.weights[vertex.startWeight+i];
			const FrameJoint &joint = curSkeleton[weight.jointIndex];
			
			vec3 tempPos = joint.orientation * weight.position + joint.position;
			pos += tempPos * weight.weightBias;			
		}

		updatedBuffer.push_back(pos.x);
		updatedBuffer.push_back(pos.y);
		updatedBuffer.push_back(pos.z);
	}

	// Update the VBO
	glBindBuffer(GL_ARRAY_BUFFER, renderMesh.hVBO);
	GLsizei size = updatedBuffer.size() * sizeof(GLfloat);
	glBufferData(GL_ARRAY_BUFFER, size, &updatedBuffer[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void renderMeshes() {	
	glUseProgram(hMeshProgram);
	MVP = projection * view * model;
	GLint location = glGetUniformLocation(hSimpleProgram, "MVP");
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(MVP));

	// Start wireframe rendering
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	for(auto &mesh : g_Meshes) {
		glBindVertexArray(mesh.hVAO);
		glBindBuffer(GL_ARRAY_BUFFER, mesh.hVBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.hIndexBuffer);
		
		GLsizei count = mesh.mesh.triangles.size() * 3;
		
		updateVertexPositions(mesh);
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, 0);		
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void render() {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderMeshes();
	renderSkeleton();

	glutSwapBuffers();
}

void onTimerTick(int value) {
	yRotation += 5.0f;
	//model = glm::rotate(mat4(), yRotation, vec3(0, 1, 0)); 
	//model = glm::rotate(model, -90.0f, vec3(1, 0, 0));
	//model = glm::rotate(mat4(), -90.0f, vec3(1, 0, 0));
	
	elapsedTime += kTimerPeriod;
	int temp = (elapsedTime / 24) % 140;
	
	if(temp != curFrame) {
		curFrame = temp;
		//cout << "Frame: " << curFrame << endl;
		frameChanged = true;
	}

	glutTimerFunc(kTimerPeriod, onTimerTick, 0);
	glutPostRedisplay();
}

int main(int argc, char **argv) {
	Md5Reader reader;
	const string meshFilename("Boblamp/boblampclean.md5mesh");
	const string animFilename("Boblamp/boblampclean.md5anim");

	try {
		 g_MD5_VO = reader.parse(meshFilename, animFilename);
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
	setUpMeshRendering();
	setUpMeshShader();
	setUpCamera();

	glutDisplayFunc(render);
	glutTimerFunc(kTimerPeriod, onTimerTick, 0);
	glutMainLoop();

	return 0;
}