#version 330 core

out vec4 frag_color;

in  vec3 local_pos;

uniform sampler2D equirectangular_map;

const vec2 kInvActan = vec2(0.15991, 0.3183);

vec2 SamplerShpericalMap(vec3 v){
	vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
	uv *= kInvActan;
	uv += 0.5;
	return uv;
}

void main(){
	vec2 uv = SamplerShpericalMap(normalize(local_pos));
	vec3 color = texture(equirectangular_map, uv).rgb;

	frag_color = vec4(color, 1.0f);
}