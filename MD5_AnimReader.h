#ifndef MD5_ANIM_READER_H
#define MD5_ANIM_READER_H

#include <fstream>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

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

class MD5_AnimReader {
public:
	MD5_AnimInfo parse(const std::string &filename);
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