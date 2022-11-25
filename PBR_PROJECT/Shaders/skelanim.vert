#version 430 core

layout(location = 0) in vec3  pos;
layout(location = 1) in vec3  norm;
layout(location = 2) in vec2  tex;
layout(location = 5) in ivec4 bone_ids; 
layout(location = 6) in vec4  weights;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;

const int kMaxBones = 100;
const int kMaxBoneInfluence = 4;

uniform mat4 bone_matrices_arr[kMaxBones];

out vec2 texcoords;

void main(){
	vec4 pos_sum = vec4(0.0f);
	for(uint i = 0; i < kMaxBoneInfluence; i++){
		if(bone_ids[i] == -1) 
			continue;
		if(bone_ids[i] >= kMaxBones){
			pos_sum = vec4(pos, 1.0f);
			break;
		}
		vec4 local_pos  = bone_matrices_arr[bone_ids[i]] * vec4(pos, 1.0f);
		pos_sum += local_pos * weights[i];
		// the step use for ?
		vec3 local_norm = mat3(bone_matrices_arr[bone_ids[i]]) * norm;
	}

	gl_Position = proj * view * pos_sum;
	texcoords   = tex;
	
}