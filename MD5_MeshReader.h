#ifndef MD5_MESH_READER
#define MD5_MESH_READER

#include "AnimCore.h"

#include <fstream>
#include <string>
#include <vector>

struct MD5_Vertex {
	float u;
	float v;
	int startWeight;
	int weightCount;
};

struct MD5_Triangle {
	unsigned short indices[3];
};

struct MD5_Weight {
	int jointIndex;
	float weightBias;
	glm::vec3 position;
};

struct MD5_Mesh {
	std::vector<MD5_Vertex> vertices;
	std::vector<MD5_Triangle> triangles;
	std::vector<MD5_Weight> weights;
	std::string textureFilename;
};

struct MD5_MeshInfo {
	std::vector<Joint> joints;
	std::vector<MD5_Mesh> meshes;
};

class MD5_MeshReader {
public:
	MD5_MeshInfo parse(const std::string &filename);
private:
	void processVersion();
	void processCommandLine();
	void processJointsAndMeshCounts();
	void processJoints();
	void processMeshes();
	void processMesh();

	void buildJoint(const std::string &line, int count);
	glm::mat4 getChildToParentMatrix(const Joint &joint);
	void computeJointToWorld(Joint &joint);
	void computeWComponent(Joint &joint);
private:
	std::ifstream mMeshFile;
	std::vector<Joint> mJoints;

	MD5_Mesh mCurMesh;
	std::vector<MD5_Mesh> mMeshes;
};

#endif