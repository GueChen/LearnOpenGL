#ifndef __BONE_H
#define __BONE_H

#include <glm/glm.hpp>

struct BoneInfo {
	int		  id;		// matrices ids
	glm::mat4 offset;	// offset from global to local
};

#endif // !__BONE_H
