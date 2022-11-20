#version 330 core

out vec4 frag_color;

in vec3 local_pos;

uniform samplerCube env_map;

void main(){
	vec3 env_color = texture(env_map, local_pos).rgb;
	env_color  = pow(env_color / (env_color + vec3(1.0)), vec3(1.0 / 2.2));
	frag_color = vec4(env_color, 1.0f);
}




