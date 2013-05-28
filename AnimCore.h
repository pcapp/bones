#ifndef ANIM_CORE_H
#define ANIM_CORE_H

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct Joint {
	std::string name;
	glm::vec3 position;
	glm::quat orientation;
	glm::mat4 jointToWorld;
	// No scale for now
	int parentIndex;
};

struct Skeleton {
	std::vector<Joint> joints;
};


#endif