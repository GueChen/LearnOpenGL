#version 330 core

layout (location = 0) in vec3 apos;
layout (location = 1) in vec3 anormal;
layout (location = 2) in vec2 atexcoords;

out vec2 tex_coords;
out vec3 world_pos;
out vec3 normal;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;

void main(){
	tex_coords = atexcoords;
	world_pos  = vec3(model  * vec4(apos, 1.0));
	normal	   = mat3(model) * anormal;

	gl_Position = proj * view * vec4(world_pos, 1.0);
}