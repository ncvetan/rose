#version 460 core

in vs_data {
	vec3 frag_pos;
	vec3 normal;
	vec2 tex_coords;
} vs_in;

struct DirLight {
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

layout (std140, binding = 2) uniform globals
{
	mat4 projection;     // 64
	mat4 view;		     // 128
	vec3 camera_pos;     // 144
	DirLight dir_light;  // 208
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

#define N_POINT_LIGHTS 2
uniform PointLight point_lights[N_POINT_LIGHTS];

out vec4 frag_color;

vec3 calc_dir_light(DirLight light, vec3 normal, vec3 view_dir) {
	
	// ambient
	vec3    ambient_rgb = vec3(0.0);
	vec3    light_dir = normalize(-light.direction);
	for (int i = 0; i < N_MATS; ++i){
		ambient_rgb += texture(materials[i].diffuse, vs_in.tex_coords).rgb;
	}
	ambient_rgb *= light.ambient;

	// diffuse
	vec3	diffuse_rgb = vec3(0.0);
	float	diffuse_strength = max(dot(normal, light_dir), 0.0);
	for (int i = 0; i < N_MATS; ++i){
		diffuse_rgb += texture(materials[i].diffuse, vs_in.tex_coords).rgb;
	}
	diffuse_rgb *= light.diffuse * diffuse_strength;
	
	// specular
	vec3	specular_rgb;
	vec3	reflect_dir = reflect(-light_dir, normal);
	for (int i = 0; i < N_MATS; ++i){
		float	specular_strength = pow(max(dot(view_dir, reflect_dir), 0.0), materials[i].shine);
		specular_rgb += texture(materials[i].specular, vs_in.tex_coords).rgb * specular_strength;
	}
	specular_rgb *= light.specular;

	return  (ambient_rgb + diffuse_rgb + specular_rgb);
}

vec3 calc_point_light(PointLight light, vec3 normal, vec3 view_dir, vec3 frag_pos) {
	
	vec3    light_dir = normalize(light.position - frag_pos);
	vec3    ambient_rgb = light.ambient;
	
	for (int i = 0; i < N_MATS; ++i){
		ambient_rgb *= texture(materials[i].diffuse, vs_in.tex_coords).rgb;
	}

	float	diffuse_strength = max(dot(normal, light_dir), 0.0);
	vec3	diffuse_rgb = light.diffuse * diffuse_strength;

	for (int i = 0; i < N_MATS; ++i){
		diffuse_rgb *= texture(materials[i].diffuse, vs_in.tex_coords).rgb;
	}

	vec3	reflect_dir = reflect(-light_dir, normal);
	vec3	specular_rgb = light.specular;

	for (int i = 0; i < N_MATS; ++i){
		float	specular_strength = pow(max(dot(view_dir, reflect_dir), 0.0), materials[i].shine);
		specular_rgb *= texture(materials[i].specular, vs_in.tex_coords).rgb * specular_strength;
	}

	float   d = length(light.position - frag_pos);
	float   attenuation = 1.0 / (light.attn_const + light.attn_lin * d + light.attn_quad * pow(d, 2));

	return vec3((ambient_rgb + diffuse_rgb + specular_rgb) * attenuation);
}

void main() {

	vec3 normal = normalize(vs_in.normal);
	vec3 view_dir = normalize(camera_pos - vs_in.frag_pos);
	vec3 result = calc_dir_light(dir_light, normal, view_dir);

	//	for (int i = 0; i < N_POINT_LIGHTS; ++i){
	//		result += calc_point_light(point_lights[i], norm, view_dir, normalize(vs_in.frag_pos));
	//	}

	frag_color = vec4(result, 1.0);
}