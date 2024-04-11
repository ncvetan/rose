#version 400 core

in vec3 frag_pos;
in vec3 normal;
in vec2 tex_coords;

struct Material {
	sampler2D	diffuse_map;
	sampler2D	specular_map;
	float		shine_factor;
};

struct DirLight {
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
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

uniform vec3 view_pos;

#define N_MATS 1
uniform Material materials[N_MATS];

uniform DirLight dir_light;

#define N_POINT_LIGHTS 1
uniform PointLight point_lights[N_POINT_LIGHTS];
uniform SpotLight spot_light;

out vec4 frag_color;

vec3 calc_dir_light(DirLight light, vec3 normal, vec3 view_dir) {
	
	vec3    light_dir = normalize(-light.direction);
	vec3    ambient_rgb = light.ambient;
	
	for (int i = 0; i < N_MATS; ++i){
		ambient_rgb *= texture(materials[i].diffuse_map, tex_coords).rgb;
	}

	float	diffuse_strength = max(dot(normal, light_dir), 0.0);
	vec3	diffuse_rgb = light.diffuse * diffuse_strength;

	for (int i = 0; i < N_MATS; ++i){
		diffuse_rgb *= texture(materials[i].diffuse_map, tex_coords).rgb;
	}

	vec3	reflect_dir = reflect(-light_dir, normal);
	vec3	specular_rgb = light.specular;

	for (int i = 0; i < N_MATS; ++i){
		float	specular_strength = pow(max(dot(view_dir, reflect_dir), 0.0), materials[i].shine_factor);
		specular_rgb *= texture(materials[i].specular_map, tex_coords).rgb * specular_strength;
	}

	return  (ambient_rgb + diffuse_rgb + specular_rgb);
}

vec3 calc_point_light(PointLight light, vec3 normal, vec3 view_dir, vec3 frag_pos) {
	
	vec3    light_dir = normalize(light.position - frag_pos);
	vec3    ambient_rgb = light.ambient;
	
	for (int i = 0; i < N_MATS; ++i){
		ambient_rgb *= texture(materials[i].diffuse_map, tex_coords).rgb;
	}

	float	diffuse_strength = max(dot(normal, light_dir), 0.0);
	vec3	diffuse_rgb = light.diffuse * diffuse_strength;

	for (int i = 0; i < N_MATS; ++i){
		diffuse_rgb *= texture(materials[i].diffuse_map, tex_coords).rgb;
	}

	vec3	reflect_dir = reflect(-light_dir, normal);
	vec3	specular_rgb = light.specular;

	for (int i = 0; i < N_MATS; ++i){
		float	specular_strength = pow(max(dot(view_dir, reflect_dir), 0.0), materials[i].shine_factor);
		specular_rgb *= texture(materials[i].specular_map, tex_coords).rgb * specular_strength;
	}

	float   d = length(light.position - frag_pos);
	float   attenuation = 1.0 / (light.attn_const + light.attn_lin * d + light.attn_quad * pow(d, 2));

	return vec3((ambient_rgb + diffuse_rgb + specular_rgb) * attenuation);
}

void main() {

	vec3 norm = normalize(normal);
	vec3 view_dir = normalize(view_pos - frag_pos);
	vec3 result = calc_dir_light(dir_light, norm, view_dir);

	for (int i = 0; i < N_POINT_LIGHTS; ++i){
		result += calc_point_light(point_lights[i], norm, view_dir, normalize(frag_pos));
	}

	frag_color = vec4(result, 1.0);
}