#ifndef ANIM_CORE_H
#define ANIM_CORE_H

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct Joint {
	glm::vec3 position;
	glm::quat orientation;
	int parentIndex;
};

struct Skeleton {
	std::vector<Joint> joints;
};


#endif