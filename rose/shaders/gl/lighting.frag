#version 460 core

layout (location = 0) out vec4 frag_color;
layout (location = 1) out vec4 brightness_color;

in vs_data {
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

uniform sampler2D gbuf_pos;
uniform sampler2D gbuf_norms;
uniform sampler2D gbuf_colors;

struct PointLight {
	vec3 pos;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float attn_const;
	float intensity;
};

#define N_POINT_LIGHTS 2
uniform PointLight point_lights[N_POINT_LIGHTS];

vec3 calc_dir_light(DirLight light, vec3 frag_pos, vec3 normal, float spec) {
	// ambient
	vec3 ambient_rgb = light.ambient;

	// diffuse
	float diffuse_strength = max(dot(-normalize(light.direction), normal), 0.0);
	vec3 diffuse_rgb = light.diffuse * diffuse_strength;

	// specular
	vec3 specular_rgb = light.specular;
	float specular_strength = 0.0;

	if (diffuse_strength != 0.0) {
		vec3 view_dir = normalize(camera_pos - frag_pos);
		vec3 half_dir = normalize(light.direction + view_dir);
		specular_strength = pow(max(dot(view_dir, half_dir), 0.0), 16);
	}
	specular_rgb *= specular_strength;

	return (ambient_rgb + diffuse_rgb + spec * specular_rgb);
};

vec3 calc_point_light(PointLight light, vec3 frag_pos, vec3 normal, float spec) {
	// ambient
	vec3 ambient_rgb = light.ambient;

	// diffuse
	vec3 light_dir = normalize(light.pos - frag_pos);
	float diffuse_strength = max(dot(light_dir, normal), 0.0);
	vec3 diffuse_rgb = light.diffuse * diffuse_strength;

	// specular
	vec3 specular_rgb = light.specular;
	float specular_strength = 0.0;

	if (diffuse_strength != 0.0) {
		vec3 view_dir = normalize(camera_pos - frag_pos);
		vec3 half_dir = normalize(light_dir + view_dir);
		specular_strength = pow(max(dot(view_dir, half_dir), 0.0), 16);

	}
	specular_rgb *= specular_strength;

	float d = length(light.pos - frag_pos);
	float attenuation = 1.0 / (1.0 + light.attn_const * d * d);

	return (ambient_rgb + diffuse_rgb + spec * specular_rgb) * attenuation * light.intensity;
	
	// TODO: Reimplement shadows
	// float shadow = calc_point_shadow(fs_in.frag_pos_ws, point_lights[0].pos, shadow_map, far_plane);
	// return vec3(ambient_rgb + (1.0 - shadow) * (diffuse_rgb + specular_rgb)) * attenuation * light.intensity;
}

void main() {
	
	vec3 frag_pos = texture(gbuf_pos, fs_in.tex_coords).rgb;
	vec3 norm = texture(gbuf_norms, fs_in.tex_coords).rgb;
	vec3 color = texture(gbuf_colors, fs_in.tex_coords).rgb;
	float spec = texture(gbuf_colors, fs_in.tex_coords).w;

	vec3 result = color * calc_dir_light(dir_light, frag_pos, norm, spec);

	for (int i = 0; i < N_POINT_LIGHTS; ++i) {
		result += calc_point_light(point_lights[i], frag_pos, norm, spec);
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