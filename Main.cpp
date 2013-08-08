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

#include "Shader.h"
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

struct FrameJoint {
	string name;
	vec3 position;
	quat orientation;
	int parentIndex;
};

struct RenderableMesh {
	MD5_Mesh mesh;
	GLuint hVBO;
	GLuint hVAO;
	GLuint hIndexBuffer;
};

////////////////////////
// GLOBALS
////////////////////////
MD5_VO g_MD5_VO;

Shader *g_pPassthroughShader;
Shader *g_pMeshShader;

// For rendering the joints
GLuint g_hJointVAO;
GLuint g_hJointIndexBuffer;

float yRotation = 0.0f;

mat4 g_model;
mat4 g_view;
mat4 g_projection;
mat4 g_MVP;

GLuint g_hSkeletonBuffer;
GLuint g_hSkeletonVAO;
GLuint g_numBonesToDraw;

Bounds g_bounds;

bool frameChanged = true;
int curFrame = 0;
int elapsedTime = 0;
GLuint hVerticesBuffer;
GLuint hColorsBuffer;

const int kTimerPeriod = 50;
const float kFovY = 45.0f;

vector<vector<FrameJoint>> frameSkeletons;
vector<RenderableMesh> g_Meshes;

bool buildShaders() {
	g_pPassthroughShader = new Shader();
	g_pPassthroughShader->compile("simple.vert", GL_VERTEX_SHADER);
	g_pPassthroughShader->compile("simple.frag", GL_FRAGMENT_SHADER);
	
	glBindAttribLocation(g_pPassthroughShader->handle(), 0, "VertexPosition");
	glBindAttribLocation(g_pPassthroughShader->handle(), 1, "VertexRGBA");

	g_pPassthroughShader->link();

	g_pMeshShader = new Shader();
	g_pMeshShader->compile("mesh.vert", GL_VERTEX_SHADER);
	g_pMeshShader->compile("mesh.frag", GL_FRAGMENT_SHADER);
	
	glBindAttribLocation(g_pMeshShader->handle(), 0, "VertexPosition");
	g_pMeshShader->link();

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
	
	CalculateBounds(&vertices[0], vertices.size(), g_bounds);

	// Set up the vertex buffer object
	glGenBuffers(1, &hVerticesBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, hVerticesBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), &vertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &hColorsBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, hColorsBuffer);
	glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GLfloat), &colors[0], GL_STATIC_DRAW);

	// Set up the vertex array object
	glGenVertexArrays(1, &g_hJointVAO);
	glBindVertexArray(g_hJointVAO);

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
	
	glGenBuffers(1, &g_hJointIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_hJointIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), &indices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void setUpSkeletonRendering() {
	vector<GLfloat> vertexData;
	g_numBonesToDraw = 0;

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

			++g_numBonesToDraw;
		}
	}

	glGenBuffers(1, &g_hSkeletonBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, g_hSkeletonBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(GLfloat), &vertexData[0], GL_STATIC_DRAW);

	// Create the VAO
	glGenVertexArrays(1, &g_hSkeletonVAO);
	glBindVertexArray(g_hSkeletonVAO);

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
	g_numBonesToDraw = 0;

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

			++g_numBonesToDraw;
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, g_hSkeletonBuffer);
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

void setUpCamera() {
	// Set up the camera to frame the model. 
	g_projection = glm::perspective(kFovY, 4.0f/3.0f, 0.1f, 1000.0f);
	
	float modelLen = max(g_bounds.minY, g_bounds.maxY);
	float totalLen = modelLen / 0.8f;
	float half_fov = kFovY / 2.0f; // Normally done by our camera

	float radians = half_fov * 3.1459895f / 180.0f;
	float z = totalLen / tan(radians);
	 
	//view = glm::translate(mat4(), vec3(0, 0, -z));
	g_view = glm::translate(mat4(), vec3(0, -20, -120));
	
	// temp
	g_model = glm::rotate(mat4(), -90.0f, vec3(1.0, 0.0, 0.0));

	glPointSize(5.0f);
}

void renderSkeleton() {
	if(frameChanged) {
		updateJointData();
		updateSkeletonData();
		frameChanged = false;
	}

	glUseProgram(g_pPassthroughShader->handle());
	g_MVP = g_projection * g_view * g_model;
	GLint location = glGetUniformLocation(g_pPassthroughShader->handle(), "MVP");
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(g_MVP));
	glBindVertexArray(g_hJointVAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_hJointIndexBuffer);
	glDrawElements(GL_POINTS, g_MD5_VO.animations[0].baseframeJoints.size(), GL_UNSIGNED_SHORT, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindVertexArray(g_hSkeletonVAO);
	glBindBuffer(GL_ARRAY_BUFFER, g_hSkeletonBuffer);
	glDrawArrays(GL_LINES, 0, g_numBonesToDraw * 2);
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
	glUseProgram(g_pMeshShader->handle());
	g_MVP = g_projection * g_view * g_model;
	GLint location = glGetUniformLocation(g_pPassthroughShader->handle(), "MVP");
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(g_MVP));

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

	cout << "Loading the model data." << endl;
	try {
		 g_MD5_VO = reader.parse(meshFilename, animFilename);
	}
	catch(exception &e) {
		cout << e.what() << endl;
		return -1;
	}
	
	cout << "Initializing GLUT." << endl;
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

	if(!buildShaders()) {
		cout << "Unable to build the shader program." << endl;
		return -1;
	}

	cout << "Created the shader and loaded the mesh." << endl;
	createFrameSkeletons();
	setUpModel();
	setUpSkeletonRendering();
	setUpMeshRendering();
	setUpCamera();

	glutDisplayFunc(render);
	glutTimerFunc(kTimerPeriod, onTimerTick, 0);
	glutMainLoop();

	return 0;
}