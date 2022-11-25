#version 430 core

out vec4 frag_color;

in  vec2 texcoords;

uniform sampler2D texture_diffuse1;

void main(){
	frag_color = texture(texture_diffuse1, texcoords);
}