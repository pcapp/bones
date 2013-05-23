#ifndef MD5READER_H
#define MD5READER_H

#include <string>
#include <fstream>
#include "AnimCore.h"

struct JointInfo {
	std::string name;
	int parent;
	int flags;
	int startIndex;
};

struct BaseframeJoint {
	glm::vec3 position;
	glm::quat orientation;
	glm::mat4 jointToWorld;
};

struct MD5_AnimInfo {
	std::vector<BaseframeJoint> baseframeJoints;
	std::vector<JointInfo> jointsInfo;
	std::vector<std::vector<float>> framesData;
	int numFrames;

	// temp
	std::vector<Joint> joints;
};

class Md5Reader {
public:
	MD5_AnimInfo parse(const std::string &meshFilename, const std::string &animFilename);
private:
	void processVersion();
	void processCommandLine();
	void processJointsAndMeshCounts();
	void processJoints();
	void buildJoint(const std::string &line, int count);
	glm::mat4 getChildToParentMatrix(const Joint &joint);
	void computeJointToWorld(Joint &joint);
	void computeWComponent(Joint &joint);

	void processAnimHeader();
	void processHierarchy();
	void processBounds();
	void processBaseframeJoints();
	void processFramesData();
private:
	int mNumFrames;
	int mFrameRate;
	std::vector<JointInfo> mJointsInfo;
	std::vector<BaseframeJoint> mBaseframeJoints;
	std::vector<std::vector<float>> mFramesData;
	std::ifstream mMeshFile;
	std::ifstream mAnimFile;
	std::vector<Joint> mJoints;
};

#endif