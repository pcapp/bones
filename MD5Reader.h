#ifndef MD5READER_H
#define MD5READER_H

#include <string>
#include <fstream>
#include "AnimCore.h"
#include "MD5_MeshReader.h"

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
};

struct MD5_VO {
	MD5_MeshInfo mesh;
	std::vector<MD5_AnimInfo> animations;
};

class Md5Reader {
public:
	MD5_VO parse(const std::string &meshFilename, const std::string &animFilename);
private:
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
	
	std::ifstream mAnimFile;
};

#endif