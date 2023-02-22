#version 450 core

layout(location = 0) in vec3 world_pos;
layout(location = 1) in vec3 norm;

layout(location = 0) out vec4 frag_color;

//__________________ Primitive Related ___________________________________
uniform int   pri_tot_num;
uniform vec4  color_start;
uniform vec4  color_end;
uniform float alpha;

//__________________ Metallic Properties _________________________________
uniform float metallic;
uniform float roughness;
uniform float ao;

//__________________ Light Properties ____________________________________
uniform vec3  light_dir;
uniform vec3  light_color;

//__________________ Camera Properties ___________________________________
uniform vec3  camera_pos;

const float   Pi = 3.1415295359;

//__________________ Function Implementations ____________________________
vec3 FresnelSchlick(float cos_theta, vec3 F0, float roughness){
	return F0 + (max(vec3(1.0 - roughness), F0)- F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness){
	float a     = roughness;
	float a2    = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2= NdotH * NdotH;

	float nom   = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = Pi * denom * denom;
	
	return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float k){	
	return NdotV / (NdotV * (1 - k) / k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness){
	float a = roughness + 1.0;
	float k = (a * a) / 8.0;
	
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);

	float ggx1 = GeometrySchlickGGX(NdotV, k);
	float ggx2 = GeometrySchlickGGX(NdotL, k);

	return ggx1 * ggx2;
}

void main(){	
	vec3 N = normalize(norm);					// normal
	vec3 V = normalize(camera_pos - world_pos); // view direction
	vec3 albedo = mix(color_start, color_end, gl_PrimitiveID / float(pri_tot_num)).xyz; 
	
	vec3 L = normalize(light_dir);
	vec3 H = normalize(V + L);					// half vector direction
	
	// caculate Fresnel term
	vec3  F0 = mix(vec3(0.04), albedo, metallic);
	vec3  Lo = vec3(0.0);
	{
	vec3  Fres = FresnelSchlick(max(dot(H, V), 0.0), F0, roughness);

	// caculate Distribution Term and Geomtry Occlusion Term
	float Ndf = DistributionGGX(N, H, roughness);
	float Geo = GeometrySmith(N, V, L, roughness);

	vec3  nom      = Ndf * Fres * Geo;
	float denom    = 4 * max(dot(V, N), 0.0) * max(dot(L, N), 0.0) + 0.0001;
	vec3  specular = nom / denom;

	vec3  Ks = Fres;
	vec3  Kd = vec3(1.0 - Ks);

	float NdotL = max(dot(N, L), 0.0);
	
	Lo += (Kd * albedo / Pi + specular) * light_color * NdotL;
	}

	vec3  F = FresnelSchlick(max(dot(N, V), 0.0), F0, roughness);

	vec3  Ks = F;
	vec3  Kd = vec3(1.0) - Ks;
	Kd *= 1.0 - metallic;

	vec3 ambient = vec3(0.03) * albedo * ao;
	
	vec3 color = ambient + Lo;
	// gamma correction
	color = pow(color / (color + vec3(1.0)), vec3(1.0 / 2.2));

	frag_color  = vec4(color, alpha);
}