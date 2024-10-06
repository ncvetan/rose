#version 460 core

in vs_data {
	vec3 frag_pos;
	vec3 frag_pos_tan;
	vec3 normal;
	vec3 view_pos_tan;
	vec3 light_pos_tan;
	vec2 tex_coords;
} fs_in;

struct DirLight {
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

layout (std140, binding = 2) uniform globals {
	mat4 projection;
	mat4 view;
	vec3 camera_pos;
	DirLight dir_light;
};

struct Material {
	sampler2D	diffuse_map;
	sampler2D	specular_map;
	sampler2D	normal_map;
	float		shine;
};

struct PointLight {
	vec3 pos;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float attn_const;
	float attn_lin;
	float attn_quad;
};

#define N_MATS 1			// todo: variable support
uniform Material materials[N_MATS];

#define N_POINT_LIGHTS 1	// todo: variable support
uniform PointLight point_lights[N_POINT_LIGHTS];

uniform samplerCube shadow_map;
uniform float gamma_co;
uniform float bias;
uniform float far_plane;

out vec4 frag_color;

float calc_point_shadow(vec3 frag_pos, vec3 light_pos, samplerCube shadow_map, float far_plane) {
	vec3 frag_to_light = frag_pos - light_pos;
	float closest = texture(shadow_map, frag_to_light).r * far_plane;
	float depth = length(frag_to_light);
	float bias = 0.05;
	float shadow = (depth - bias > closest) ? 1.0 : 0.0;
	return shadow;
}

float calc_directional_shadow(vec3 frag_pos, vec3 light_dir, samplerCube shadow_map, float far_plane) {
	return 1.0;
}

vec3 calc_dir_light(DirLight light, vec3 frag_pos, vec2 tex_coords, vec3 normal) {
	// ambient
	vec3 ambient_rgb = vec3(0.0);
	for (int i = 0; i < N_MATS; ++i) {
		ambient_rgb += texture(materials[i].diffuse_map, tex_coords).rgb;
	}
	ambient_rgb *= light.ambient;

	// diffuse
	vec3 diffuse_rgb = vec3(0.0);
	float diffuse_strength = max(dot(-normalize(light.direction), normal), 0.0);
	for (int i = 0; i < N_MATS; ++i) {
		diffuse_rgb += texture(materials[i].diffuse_map, tex_coords).rgb;
	}
	diffuse_rgb *= light.diffuse * diffuse_strength;
	
	// specular
	vec3 specular_rgb = vec3(0.0);
	vec3 view_dir = normalize(fs_in.view_pos_tan - frag_pos);
	vec3 half_dir = normalize(light.direction + view_dir);
	for (int i = 0; i < N_MATS; ++i) {
		float specular_strength = pow(max(dot(view_dir, half_dir), 0.0), materials[i].shine);
		specular_rgb += texture(materials[i].specular_map, tex_coords).rgb * specular_strength;
	}
	specular_rgb *= light.specular;

	return (ambient_rgb + diffuse_rgb + specular_rgb);
}

vec3 calc_point_light(PointLight light, vec3 frag_pos, vec2 tex_coords, vec3 normal) {
	// ambient
	vec3 ambient_rgb = vec3(0.0);
	for (int i = 0; i < N_MATS; ++i){
		ambient_rgb += texture(materials[i].diffuse_map, tex_coords).rgb;
	}
	ambient_rgb *= light.ambient;

	// diffuse
	// TODO: Temporarily using tangest space light pos here, change to support multiple lights
	vec3 light_dir = normalize(fs_in.light_pos_tan - frag_pos);
	vec3 diffuse_rgb = vec3(0.0);
	float diffuse_strength = max(dot(light_dir, normal), 0.0);
	for (int i = 0; i < N_MATS; ++i) {
		diffuse_rgb += texture(materials[i].diffuse_map, tex_coords).rgb;
	}
	diffuse_rgb *= light.diffuse * diffuse_strength;

	// specular
	vec3 specular_rgb = vec3(0.0);
	vec3 view_dir = normalize(fs_in.view_pos_tan - frag_pos);
	vec3 half_dir = normalize(light_dir + view_dir);
	for (int i = 0; i < N_MATS; ++i) {
		float specular_strength = pow(max(dot(view_dir, half_dir), 0.0), materials[i].shine);
		specular_rgb += texture(materials[i].specular_map, tex_coords).rgb * specular_strength;
	}
	specular_rgb *= light.specular;

	float d = length(fs_in.light_pos_tan - frag_pos);
	float attenuation = 1.0 / (1.0 + d * d);
	float shadow = calc_point_shadow(fs_in.frag_pos, point_lights[0].pos, shadow_map, far_plane);

	return vec3(ambient_rgb + (1.0 - shadow) * (diffuse_rgb + specular_rgb)) * attenuation;
}

void main() {
	
	vec3 normal = texture(materials[0].normal_map, fs_in.tex_coords).rgb;
	normal = normalize(normal * 2.0 - 1.0);
	
	vec3 result = calc_dir_light(dir_light, fs_in.frag_pos_tan, fs_in.tex_coords, normal);
	
	for (int i = 0; i < N_POINT_LIGHTS; ++i) {
		result += calc_point_light(point_lights[i], fs_in.frag_pos_tan, fs_in.tex_coords, normal);
	}

	result = pow(result, vec3(1.0 / gamma_co));  // gamma correction
	frag_color = vec4(result, 1.0);
}