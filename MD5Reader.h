#ifndef MD5READER_H
#define MD5READER_H

#include <string>
#include <fstream>
#include "AnimCore.h"

struct AnimInfo {
	Skeleton skeleton;
};

struct JointInfo {
	std::string name;
	int parent;
	int flags;
	int startIndex;
};

class Md5Reader {
public:
	AnimInfo parse(const std::string &meshFilename, const std::string &animFilename);
private:
	void processVersion();
	void processCommandLine();
	void processJointsAndMeshCounts();
	void processJoints();
	void buildJoint(const std::string &line, int count);
	glm::mat4 getChildToParentMatrix(const Joint &joint);
	void computeJointToWorld(Joint &joint);

	void processAnimHeader();
	void processHierarchy();
private:
	int mNumFrames;
	int mFrameRate;
	std::vector<JointInfo> mJointsInfo;
	std::vector<std::vector<float>> mFramesData;
	std::ifstream mMeshFile;
	std::ifstream mAnimFile;
	std::vector<Joint> mJoints;
};

#endif