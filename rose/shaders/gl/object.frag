#version 460 core

in vs_data {
	vec3 frag_pos;
	vec4 frag_pos_light_space;
	vec3 normal;
	vec2 tex_coords;
} fs_in;

struct DirLight {
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

layout (std140, binding = 2) uniform globals
{
	mat4 projection;
	mat4 view;
	vec3 camera_pos;
	DirLight dir_light;
};

struct Material {
	sampler2D	diffuse;
	sampler2D	specular;
	float		shine;
};

struct PointLight {
	vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float attn_const;
	float attn_lin;
	float attn_quad;
};

#define N_MATS 1
uniform Material materials[N_MATS];

#define N_POINT_LIGHTS 1
uniform PointLight point_lights[N_POINT_LIGHTS];

uniform sampler2D shadow_map;
uniform float gamma_co;
uniform float bias;

out vec4 frag_color;

float calc_shadow() {
	vec3 coords = fs_in.frag_pos_light_space.xyz / fs_in.frag_pos_light_space.w;
	coords = coords * 0.5 + 0.5;  // put into NDC
	float depth = texture(shadow_map, coords.xy).r;
	// if(coords.z > 1.0) return 0.0;  // 
	float shadow = (coords.z - bias > depth) ? 1.0 : 0.0;
	return shadow;
}

vec3 calc_dir_light(DirLight light, vec3 normal) {
	// ambient
	vec3 ambient_rgb = vec3(0.0);
	for (int i = 0; i < N_MATS; ++i){
		ambient_rgb += texture(materials[i].diffuse, fs_in.tex_coords).rgb;
	}
	ambient_rgb *= light.ambient;

	// diffuse
	vec3 diffuse_rgb = vec3(0.0);
	float diffuse_strength = max(dot(-normalize(light.direction), normal), 0.0);
	for (int i = 0; i < N_MATS; ++i){
		diffuse_rgb += texture(materials[i].diffuse, fs_in.tex_coords).rgb;
	}
	diffuse_rgb *= light.diffuse * diffuse_strength;
	
	// specular
	vec3 specular_rgb = vec3(0.0);
	vec3 view_dir = normalize(camera_pos - fs_in.frag_pos);
	vec3 half_dir = normalize(light.direction + view_dir);
	for (int i = 0; i < N_MATS; ++i){
		float specular_strength = pow(max(dot(view_dir, half_dir), 0.0), materials[i].shine);
		specular_rgb += texture(materials[i].specular, fs_in.tex_coords).rgb * specular_strength;
	}
	specular_rgb *= light.specular;

	float shadow = calc_shadow();

	return (ambient_rgb + (1.0 - shadow) * (diffuse_rgb + specular_rgb));
}

vec3 calc_point_light(PointLight light, vec3 normal) {
	// ambient
	vec3 ambient_rgb = vec3(0.0);
	for (int i = 0; i < N_MATS; ++i){
		ambient_rgb += texture(materials[i].diffuse, fs_in.tex_coords).rgb;
	}
	ambient_rgb *= light.ambient;

	// diffuse
	vec3 light_dir = normalize(light.position - fs_in.frag_pos);
	light_dir.z = -light_dir.z;
	vec3 diffuse_rgb = vec3(0.0);
	float diffuse_strength = max(dot(light_dir, normal), 0.0);
	for (int i = 0; i < N_MATS; ++i){
		diffuse_rgb += texture(materials[i].diffuse, fs_in.tex_coords).rgb;
	}
	diffuse_rgb *= light.diffuse * diffuse_strength;

	// specular
	vec3 specular_rgb = vec3(0.0);
	vec3 view_dir = normalize(camera_pos - fs_in.frag_pos);
	vec3 half_dir = normalize(-light_dir + view_dir);
	for (int i = 0; i < N_MATS; ++i){
		float specular_strength = pow(max(dot(view_dir, half_dir), 0.0), materials[i].shine);
		specular_rgb += texture(materials[i].specular, fs_in.tex_coords).rgb * specular_strength;
	}
	specular_rgb *= light.specular;

	float d = length(light.position - fs_in.frag_pos);
	float attenuation = 1.0 / (1.0 + d * d);

	return vec3(ambient_rgb + diffuse_rgb + specular_rgb) * attenuation;
}

void main() {

	vec3 normal = normalize(fs_in.normal);
	vec3 result = calc_dir_light(dir_light, normal);

	for (int i = 0; i < N_POINT_LIGHTS; ++i){
		result += calc_point_light(point_lights[i], normal);
	}

	result = pow(result, vec3(1.0 / gamma_co));  // gamma correction
	frag_color = vec4(result, 1.0);
}