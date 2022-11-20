#version 330 core

layout (location = 0) in vec3 apos;

out vec3 local_pos;

uniform mat4 proj;
uniform mat4 view;

void main(){
	local_pos   = apos;
	gl_Position = proj * view * vec4(apos, 1.0f);
}