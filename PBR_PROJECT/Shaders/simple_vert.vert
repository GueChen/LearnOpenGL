#version 450 core

layout(location = 0) in vec3 apos;
layout(location = 1) in vec3 anorm;

layout(location = 0) out vec3 world_pos;
layout(location = 1) out vec3 normal;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;

void main(){
	world_pos = vec3(model * vec4(apos, 1.0));
	normal    = mat3(transpose(inverse(model))) * anorm;
	gl_Position = proj * view * vec4(world_pos, 1.0);
}