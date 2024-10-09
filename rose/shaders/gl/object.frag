#version 460 core

in vs_data {
	vec3 frag_pos_ws;
	vec3 frag_pos_ts;
	vec3 normal;
	vec3 view_pos_ts;
	vec3 light_pos_ts;
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
	sampler2D	displace_map;
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

vec2 parallax_mapping(vec3 view_dir, float num_layers, sampler2D displace_map) { 
	
	float layer_depth = 1.0 / num_layers;	// depth of each layer
	float curr_layer_depth = 0.0f;			// depth value of the search

	float height = texture(displace_map, fs_in.tex_coords).r;
	vec2 delta_coords = (view_dir.xy * height * 0.1) / num_layers;
	
	vec2 curr_tex_coords = fs_in.tex_coords;
	float curr_depth_val = texture(displace_map, curr_tex_coords).r;

	// search for first layer where sample height is greater than layer height
	while (curr_layer_depth < curr_depth_val) {
		curr_tex_coords -= delta_coords;
		curr_depth_val = texture(displace_map, curr_tex_coords).r;
		curr_layer_depth += layer_depth;
	}
	
	vec2 last_tex_coords = curr_tex_coords + delta_coords;
	
	// linear interpolation of tex coords
	float after = curr_depth_val - curr_layer_depth;
	float before = texture(displace_map, last_tex_coords).r - curr_layer_depth + layer_depth;
	float weight = after / (after - before);
	vec2 tex_coords = (last_tex_coords * weight) + curr_tex_coords * (1.0 - weight);
	
	return tex_coords;
}

float calc_point_shadow(vec3 frag_pos, vec3 light_pos, samplerCube shadow_map, float far_plane) {
	vec3 frag_to_light = frag_pos - light_pos;
	float closest = texture(shadow_map, frag_to_light).r * far_plane;
	float depth = length(frag_to_light);
	float bias = 0.05;
	float shadow = (depth - bias > closest) ? 1.0 : 0.0;
	return shadow;
}

float calc_directional_shadow(vec3 frag_pos, vec3 light_dir, samplerCube shadow_map, float far_plane) {
	// TODO: implement
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
	vec3 view_dir = normalize(fs_in.view_pos_ts - frag_pos);
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
	vec3 light_dir = normalize(fs_in.light_pos_ts - frag_pos);
	vec3 diffuse_rgb = vec3(0.0);
	float diffuse_strength = max(dot(light_dir, normal), 0.0);
	for (int i = 0; i < N_MATS; ++i) {
		diffuse_rgb += texture(materials[i].diffuse_map, tex_coords).rgb;
	}
	diffuse_rgb *= light.diffuse * diffuse_strength;

	// specular
	vec3 specular_rgb = vec3(0.0);
	vec3 view_dir = normalize(fs_in.view_pos_ts - frag_pos);
	vec3 half_dir = normalize(light_dir + view_dir);
	for (int i = 0; i < N_MATS; ++i) {
		float specular_strength = pow(max(dot(view_dir, half_dir), 0.0), materials[i].shine);
		specular_rgb += texture(materials[i].specular_map, tex_coords).rgb * specular_strength;
	}
	specular_rgb *= light.specular;

	float d = length(fs_in.light_pos_ts - frag_pos);
	float attenuation = 1.0 / (1.0 + d * d);
	float shadow = calc_point_shadow(fs_in.frag_pos_ws, point_lights[0].pos, shadow_map, far_plane);

	return vec3(ambient_rgb + (1.0 - shadow) * (diffuse_rgb + specular_rgb)) * attenuation;
}

void main() {
	
	vec3 view_dir = normalize(fs_in.view_pos_ts - fs_in.frag_pos_ts);
	
	const float min_layers = 8.0;
	const float max_layers = 32.0;
	
	// use more/less layers depending on viewing angle
	float num_layers = mix(max_layers, min_layers, max(dot(vec3(0.0, 0.0, 1.0),
	view_dir), 0.0));

	vec2 tex_coords = parallax_mapping(view_dir, num_layers, materials[0].displace_map);

	vec3 normal = texture(materials[0].normal_map, tex_coords).rgb;
	normal = normalize(normal * 2.0 - 1.0);

	vec3 result = calc_dir_light(dir_light, fs_in.frag_pos_ts, tex_coords, normal);
	
	for (int i = 0; i < N_POINT_LIGHTS; ++i) {
		result += calc_point_light(point_lights[i], fs_in.frag_pos_ts, tex_coords, normal);
	}

	result = pow(result, vec3(1.0 / gamma_co));  // gamma correction
	frag_color = vec4(result, 1.0);
}