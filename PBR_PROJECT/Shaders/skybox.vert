#version 330 core

layout(location = 0) in vec3 apos;

uniform mat4 proj;
uniform mat4 view;

out vec3 local_pos;

void main(){
	local_pos = apos;
	vec4 clip_pos = proj * mat4(mat3(view)) * vec4(local_pos, 1.0f);
	gl_Position = clip_pos.xyww;	 
}