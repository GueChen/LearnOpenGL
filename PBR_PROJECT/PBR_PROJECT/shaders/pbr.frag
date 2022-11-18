#version 330 core

out vec4 frag_color;

in  vec2 tex_coords;
in  vec3 world_pos;
in  vec3 normal;

// material params
uniform vec3  albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;

// lights 
uniform vec3 light_pos;
uniform vec3 light_color;

uniform vec3 camera_pos;

const float PI = 3.14159265359;

vec3 FresnelSchlick(float cos_theta, vec3 F0){
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness){
	float a      = roughness * roughness;
	float a2     = a * a;
	float NdotH  = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float nom    = a2;
	float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float k){	
	float nom   = NdotV;
	float denom = NdotV * (1 - k) + k;

	return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness){
	float a = roughness + 1.0;
	float k = a * a / 8;

	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);

	float ggx1  = GeometrySchlickGGX(NdotV, k);
	float ggx2  = GeometrySchlickGGX(NdotL, k);

	return ggx1 * ggx2;
}

void main()
{
	vec3  N = normalize(normal);							// normal vector
	vec3  V = normalize(camera_pos - world_pos);			// lookat vector
	vec3  Lo = vec3(0.0);									// output radiance

	// caculate the irrandiance
	vec3  L = normalize(light_pos - world_pos);				// incident vector
	vec3  H = normalize(V + L);								// halfway vector
	float light_distance = length(light_pos - world_pos);
	float attenuation    = 1.0 / (light_distance * light_distance);
	vec3  radiance		 = light_color * attenuation;

	// caculate the Fresnel term
	vec3  F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);
	vec3 Fres = FresnelSchlick(max(dot(H, V), 0.0), F0);
	
	// caculate the rest term NDF and GEOM
	float Ndf = DistributionGGX(N, H, roughness);
	float Geo = GeometrySmith(N, V, L, roughness);

	vec3  num	   = Ndf * Fres * Geo;
	float denom    = 4 * max((dot(V, N)), 0.0) * max((dot(L, N)), 0.0) + 0.0001;	// avoid all zero
	vec3  specular = num / denom;

	vec3  Ks = Fres;
	vec3  Kd = vec3(1.0) - Ks;

	// to metalic surface
	Kd *= 1.0 - metallic;
	// the last term dot 	
	float NdotL = max(dot(N, L), 0.0);

	Lo += (Kd * albedo / PI + specular) * radiance * NdotL;

	// ambient lighting
	vec3 ambient = vec3(0.03) * albedo * ao;
	vec3 color   = ambient + Lo;
	
	// linear to HDR
	color = pow(color / (color + vec3(1.0)), vec3(1.0 / 2.2));

	frag_color = vec4(color, 1.0);
}