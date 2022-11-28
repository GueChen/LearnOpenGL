#version 430 core

out vec4 frag_color;

in  vec2 texcoords;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_emission1;
uniform sampler2D texture_normal1;

void main(){
	frag_color = texture(texture_diffuse1, texcoords);
}