#ifndef MD5_MESH_READER
#define MD5_MESH_READER

#include "AnimCore.h"

#include <fstream>
#include <string>
#include <vector>

struct MD5_Mesh {

};

struct MD5_MeshInfo {
	std::vector<Joint> joints;
};

class MD5_MeshReader {
public:
	MD5_MeshInfo parse(const std::string &filename);
private:
	void processVersion();
	void processCommandLine();
	void processJointsAndMeshCounts();
	void processJoints();
	void buildJoint(const std::string &line, int count);
	glm::mat4 getChildToParentMatrix(const Joint &joint);
	void computeJointToWorld(Joint &joint);
	void computeWComponent(Joint &joint);
private:
	std::ifstream mMeshFile;
	std::vector<Joint> mJoints;
};

#endif