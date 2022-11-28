#ifndef __ANIMATOR_H
#define __ANIMATOR_H


#include "animation.h"

#include <glm/glm.hpp>

#include <vector>

class Animator {
public:
	Animator(Animation* animation):
		cur_time_(0.0f), cur_animation_(animation)
	{		
		bone_matrices_.resize(100, glm::mat4(1.0f));		
	}

	void UpdateAnimation(float dt) {
		delta_time_ = dt;
		if (cur_animation_) {
			cur_time_ += cur_animation_->GetTickPerSecond() * dt;
			cur_time_ = fmod(cur_time_, cur_animation_->GetDuration());
			CalculateBoneTransform(&cur_animation_->GetRootNode(), glm::mat4(1.0f));
		}
	}

	void PlayAnimation(Animation* p_animation) {
		cur_animation_ = p_animation;
		cur_time_ = 0.0f;
	}

	void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parent_transform) {
		std::string name = node->name;
		glm::mat4   node_transform = node->transformation;

		Bone* bone = cur_animation_->FindBone(name);

		if (bone) {
			bone->Update(cur_time_);
			node_transform = bone->GetLocalTransform();
		}

		glm::mat4 glb_transform = parent_transform * node_transform;
		auto bone_info_map = cur_animation_->GetBoneIDMap();
		if (auto iter = bone_info_map.find(name);iter != bone_info_map.end()) {		
			int idx = iter->second.id;
			glm::mat4 offset = iter->second.offset;
			bone_matrices_[idx] = glb_transform * offset;
		}

		for (int i = 0; i < node->children_count; ++i) {
			CalculateBoneTransform(&node->children[i], glb_transform);
		}
	}

	inline const std::vector<glm::mat4>& 
		GetBoneMatrices() { return bone_matrices_; }

private:
	std::vector<glm::mat4> bone_matrices_;
	Animation*			   cur_animation_;
	float				   cur_time_;
	float				   delta_time_;
};
#endif // !__ANIMATOR_H