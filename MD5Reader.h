#ifndef MD5READER_H
#define MD5READER_H

#include <string>
#include <fstream>
#include "AnimCore.h"

struct AnimInfo {
	Skeleton skeleton;
};

class Md5Reader {
public:
	AnimInfo parse(const std::string &filename);
private:
	void processVersion();
	void processCommandLine();
	void processJointsAndMeshCounts();
	void processJoints();
	void buildJoint(const std::string &line, int count);
	glm::mat4 getChildToParentMatrix(const Joint &joint);
	void computeJointToWorld(Joint &joint);
private:
	std::ifstream mFile;
	std::vector<Joint> mJoints;
};

#endif