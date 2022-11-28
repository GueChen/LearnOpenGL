#ifndef __ANIMATION_H
#define __ANIMATION_H

#include "model.h"
#include "bone.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <glm/glm.hpp>

#include <vector>
#include <string>
#include <map>

struct AssimpNodeData {
	glm::mat4   transformation;
	std::string name;
	int			children_count;
	std::vector<AssimpNodeData> children;
};

class Animation {
public: 
	Animation() = default;
	
	Animation(const std::string& anim_path, Model* model) {
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(anim_path, aiProcess_Triangulate);
		assert(scene && scene->mRootNode);
		auto animation = scene->mAnimations[0];
		duration_ = animation->mDuration;
		ticks_per_second_ = animation->mTicksPerSecond;
		ReadHeirarchyData(root_node_, scene->mRootNode);
		ReadMissingBone(animation, *model);
	}
	
	~Animation() = default;

	Bone* FindBone(const std::string& name) {
		auto iter = std::find_if(bones_.begin(), bones_.end(),
			[&](const Bone& bone) {
				return bone.GetBoneName() == name;
			});
		if (iter == bones_.end()) return nullptr;
		return &(*iter);
	}

	inline float GetTickPerSecond() const      { return ticks_per_second_; }
	inline float GetDuration()		const      { return duration_; }
	inline const AssimpNodeData& GetRootNode() { return root_node_; }
	inline const std::map<std::string, BoneInfo>& GetBoneIDMap() {
		return bone_info_map_;
	}

private:
	void ReadMissingBone(const aiAnimation* anim, Model& model) {
		int size = anim->mNumChannels;
		auto& bone_info_map = model.GetBoneInfoMap();
		int&  bone_count	= model.GetBoneCount();

		for (int i = 0; i < size; ++i) {
			auto channel = anim->mChannels[i];
			std::string bone_name = channel->mNodeName.data;
			if (bone_info_map.find(bone_name) == bone_info_map.end()) {
				bone_info_map[bone_name].id = bone_count;
				bone_count++;
			}
			bones_.push_back(Bone(channel->mNodeName.data, bone_info_map[channel->mNodeName.data].id, channel));
		}

		bone_info_map_ = bone_info_map;
	}

	void ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src) {
		assert(src);

		dest.name = src->mName.data;
		dest.transformation = toMat4(src->mTransformation);
		dest.children_count = src->mNumChildren;
		dest.children.reserve(dest.children_count);
		// dfs recursive fill datas
		for (int i = 0; i < src->mNumChildren; ++i) {
			AssimpNodeData child_data;
			ReadHeirarchyData(child_data, src->mChildren[i]);
			dest.children.push_back(child_data);
		}
	}

// Fields
// -----------------------------------------------------
private:
	float duration_;
	int	  ticks_per_second_;
	std::vector<Bone>				bones_;
	AssimpNodeData					root_node_;
	std::map<std::string, BoneInfo> bone_info_map_;
};

#endif // !__ANIMATION_H
