#ifndef __BONE_H
#define __BONE_H

#include "assimputils.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <concepts>
#include <vector>
#include <string>

struct BoneInfo {
	int		  id;		// matrices ids
	glm::mat4 offset;	// offset from global to local
};

struct KeyPosition {
	glm::vec3 pos;
	float	  timestamp;
};

struct KeyRotation {
	glm::quat ori;
	float	  timestamp;
};

struct KeyScale {
	glm::vec3 scale;
	float	  timestamp;
};

class Bone {
private:
	std::vector<KeyPosition> k_pos_;
	int kpos_size_;
	std::vector<KeyRotation> k_rot_;
	int krot_size_;
	std::vector<KeyScale>	 k_scale_;
	int kscale_size_;

	glm::mat4   local_transform_;
	std::string name_;
	int		    id_;

public:
	Bone(const std::string& name, int id, const aiNodeAnim* channel)
		:
		name_(name),
		id_(id),
		local_transform_(1.0f) {
		// insert pos data
		// -------------------------------------
		kpos_size_ = channel->mNumPositionKeys;
		k_pos_.reserve(kpos_size_);
		for (int i = 0; i < kpos_size_; ++i) {
			aiVector3D ai_pos = channel->mPositionKeys[i].mValue;
			float time_stamp = channel->mPositionKeys[i].mTime;
			KeyPosition data;
			data.pos = toVec3(ai_pos);
			data.timestamp = time_stamp;
			k_pos_.push_back(data);
		}
		// inset rot data
		// -------------------------------------
		krot_size_ = channel->mNumRotationKeys;
		k_rot_.reserve(krot_size_);
		for (int i = 0; i < krot_size_; ++i) {
			aiQuaternion ai_rot = channel->mRotationKeys[i].mValue;
			float time_stamp = channel->mRotationKeys[i].mTime;
			KeyRotation data;
			data.ori = toQuat(ai_rot);
			data.timestamp = time_stamp;
			k_rot_.push_back(data);
		}
		// inset scale data
		// -------------------------------------
		kscale_size_ = channel->mNumScalingKeys;
		k_scale_.reserve(kscale_size_);
		for (int i = 0; i < kscale_size_; ++i) {
			aiVector3D scale = channel->mScalingKeys[i].mValue;
			float time_stamp = channel->mRotationKeys[i].mTime;
			KeyScale data;
			data.scale = toVec3(scale);
			data.timestamp = time_stamp;
			k_scale_.push_back(data);
		}
	}

	void Update(float time) {
		glm::mat4 translation = InterpolatePosition(time);
		glm::mat4 rotation = InterpolateRotation(time);
		glm::mat4 scale = InterpolateScaling(time);
		local_transform_ = translation * rotation * scale;
	}

	inline glm::mat4	GetLocalTransform() { return local_transform_; }
	inline std::string  GetBoneName() const { return name_; }
	inline int			GetBoneID() { return id_; }



	template<class T>
		requires requires (T x) { x.timestamp; }
	int GetIdxFromVector(float time, std::vector<T>& vals) {
		for (int i = 0; i < vals.size() - 1; ++i) {
			if (time < vals[i + 1].timestamp) {
				return i;
			}
		}
		assert(0);
	}

private:
	float GetScaleFactor(float last_time_stamp, float next_time_stamp, float time) {
		float scale_factor = 0.0f;
		float molecule = time - last_time_stamp;
		float denominator = next_time_stamp - last_time_stamp;
		scale_factor = molecule / denominator;
		return scale_factor;
	}

	glm::mat4 InterpolatePosition(float time) {
		if (1 == kpos_size_) return glm::translate(glm::mat4(1.0f), k_pos_[0].pos);

		int prev_idx = GetIdxFromVector(time, k_pos_),
			next_idx = prev_idx + 1;

		KeyPosition& prev_pos = k_pos_[prev_idx],
			& next_pos = k_pos_[next_idx];

		float scale_factor = GetScaleFactor(prev_pos.timestamp, next_pos.timestamp, time);
		glm::vec3 ret_pos = glm::mix(prev_pos.pos, next_pos.pos, scale_factor);

		return glm::translate(glm::mat4(1.0f), ret_pos);
	}

	glm::mat4 InterpolateRotation(float time) {
		if (1 == krot_size_) {
			auto rotation = glm::normalize(k_rot_[0].ori);
			return glm::toMat4(rotation);
		}

		int prev_idx = GetIdxFromVector(time, k_rot_),
			next_idx = prev_idx + 1;
		KeyRotation& prev_rot = k_rot_[prev_idx],
			& next_rot = k_rot_[next_idx];

		float factor = GetScaleFactor(prev_rot.timestamp, next_rot.timestamp, time);
		glm::quat ret_rot = glm::normalize(glm::slerp(prev_rot.ori, next_rot.ori, factor));

		return glm::toMat4(ret_rot);
	}

	glm::mat4 InterpolateScaling(float time) {
		if (1 == kscale_size_) return glm::scale(glm::mat4(1.0f), k_scale_.front().scale);

		int prev_idx = GetIdxFromVector(time, k_scale_),
			next_idx = prev_idx + 1;
		KeyScale& prev_scale = k_scale_[prev_idx],
			& next_scale = k_scale_[next_idx];

		float factor = GetScaleFactor(prev_scale.timestamp, next_scale.timestamp, time);
		glm::vec3 ret_scale = glm::mix(prev_scale.scale, next_scale.scale, factor);
		return glm::scale(glm::mat4(1.0f), ret_scale);
	}
};

#endif // !__BONE_H
