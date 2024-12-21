#version 460 core

layout (location = 0) out vec4 frag_color;
layout (location = 1) out vec4 brightness_color;

in vs_data {
	vec2 tex_coords;
} fs_in;

struct DirLight {
	vec3 direction;
	vec3 color;
};

layout (std140, binding = 1) uniform globals {
	mat4 projection;
	mat4 view;
	vec3 camera_pos;
	DirLight dir_light;
};

uniform sampler2D gbuf_pos;
uniform sampler2D gbuf_norms;
uniform sampler2D gbuf_colors;
uniform samplerCube shadow_map;
uniform float far_plane;

// light parameters for a particular point light
struct PointLight {
	vec3 color;
	float linear;
	float quad;
	float intensity;
	float radius;
};

// global list of lights and their parameters
layout (std430, binding=5) buffer lights_ssbo {
    PointLight lights[];
};

// global list of light positions (this should always have the same length as lights_ssbo)
layout (std430, binding=6) buffer lights_pos_ssbo {
    vec4 lights_pos[];
};

float calc_point_shadow(vec3 frag_pos, vec3 light_pos, samplerCube shadow_map, float far_plane) {
	vec3 frag_to_light = frag_pos - light_pos;
	float closest = texture(shadow_map, frag_to_light).r * far_plane;
	float depth = length(frag_to_light);
	float bias = 0.05;
	float shadow = (depth - bias > closest) ? 1.0 : 0.0;
	return shadow;
}

vec3 calc_dir_light(DirLight light, vec3 frag_pos, vec3 normal, float spec) {
	// ambient
	float ambient_strength = 0.1;

	// diffuse
	float diffuse_strength = max(dot(-normalize(light.direction), normal), 0.0);

	// specular
	float specular_strength = 0.0;

	if (diffuse_strength != 0.0) {
		vec3 view_dir = normalize(camera_pos - frag_pos);
		vec3 half_dir = normalize(light.direction + view_dir);
		specular_strength = pow(max(dot(view_dir, half_dir), 0.0), 16);
	}

	return (ambient_strength + diffuse_strength + spec * specular_strength) * light.color;
};

vec3 calc_point_light(PointLight light_props, vec3 light_pos, vec3 frag_pos, vec3 normal, float spec) {
	
	float d = length(light_pos - frag_pos);
	float attenuation = 1.0 / (1.0 + light_props.linear * d + light_props.quad * d * d);

	// ambient
	float ambient_strength = 0.1;

	// diffuse
	vec3 light_dir = normalize(light_pos - frag_pos);
	float diffuse_strength = max(dot(light_dir, normal), 0.0);

	// specular
	float specular_strength = 0.0;

	if (diffuse_strength != 0.0) {
		vec3 view_dir = normalize(camera_pos - frag_pos);
		vec3 half_dir = normalize(light_dir + view_dir);
		specular_strength = pow(max(dot(view_dir, half_dir), 0.0), 16);

	}

	float shadow = calc_point_shadow(frag_pos, light_pos, shadow_map, far_plane);
	return (ambient_strength + (1.0 - shadow) * (diffuse_strength + spec * specular_strength)) * light_props.color * attenuation * light_props.intensity;
}

void main() {
	
	vec3 frag_pos = texture(gbuf_pos, fs_in.tex_coords).rgb;
	vec3 norm = texture(gbuf_norms, fs_in.tex_coords).rgb;
	vec3 color = texture(gbuf_colors, fs_in.tex_coords).rgb;
	float spec = texture(gbuf_colors, fs_in.tex_coords).w;

	vec3 result = color * calc_dir_light(dir_light, frag_pos, norm, spec);

	for (int i = 0; i < lights.length(); ++i) {
		result += calc_point_light(lights[i], vec3(lights_pos[i]), frag_pos, norm, spec);
	}

	frag_color = vec4(result, 1.0);
	
	float brightness = dot(frag_color.rgb, vec3(0.2126, 0.7152, 0.0722));	// rgb -> luminance

	if (brightness > 1.0) {
		brightness_color = vec4(frag_color.rgb, 0.0);
	}
	else {
		brightness_color = vec4(0.0, 0.0, 0.0, 0.0);
	}
}