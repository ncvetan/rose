// =============================================================================
//   applies core lighting algorithms to gbuf fragments
// =============================================================================

#version 460 core

layout (location = 0) out vec4 frag_color;

// inputs =========================================================================================

in vs_data {
	vec2 tex_coords;
} fs_in;

// struct definitions =============================================================================

// directional light properties
struct DirLight {
	vec3 direction;
	vec3 color;
};

layout (std140, binding = 1) uniform globals_ubo {
	mat4 projection;
	mat4 view;
	vec3 camera_pos;
	DirLight dir_light;
	uvec3 grid_sz;				// cluster dimensions (xyz)
	uvec2 screen_dims;			// screen [ width, height ]
	float far_z;
	float near_z;
};

// uniforms =======================================================================================

// g-buffers
uniform sampler2D gbuf_pos;
uniform sampler2D gbuf_norms;
uniform sampler2D gbuf_colors;

uniform sampler2DArray dir_shadow_maps;	 // shadow map for each cascade
uniform int n_cascades;					 // number of shadow cascades
uniform float cascade_depths[3];		 // far depth of each shadow cascade
uniform samplerCube pt_shadow_map;		 // shadow map for point lights
uniform uint pt_caster_id;				 // id of the current shadow casting point light
uniform bool ao_enabled;				 // indicates whether ambient occlusion is enabled	
uniform sampler2D occlusion_tex;		 // per-fragment occlusion values

// light parameters for a particular point light
struct PointLight {
    vec4 color;
    float radius;
    float intensity;
};

struct Cluster {
    uint count;			// number of lights affecting the cluster
    uint indices[100];	// indicies of lights up to a maximum of 100
};

// buffers ========================================================================================

// global list of lights and their parameters
layout (std430, binding=3) buffer lights_ssbo {
    PointLight light_data[];
};

// global list of light positions (this should always have the same length as lights_ssbo)
layout (std430, binding=4) buffer light_positions_ssbo {
    vec4 light_positions[];
};

// indices of lights affecting each cluster
layout (std430, binding=5) buffer clusters_ssbo {
    Cluster clusters[];
};

// contains the light space matrix for each shadow map cascade
layout(std140, binding=6) uniform light_space_mats_ubo {
	mat4 ls_mats[3];
};

// contains identifiers for each light
layout(std430, binding=7) buffer lights_ids {
	uint light_ids[];
};

// functions ======================================================================================

float calc_dir_shadow(DirLight light, vec3 pos, float frag_depth, vec3 normal) {

	vec3 res = step(vec3(cascade_depths[0], cascade_depths[1], cascade_depths[2]), vec3(abs(frag_depth)));
	int cascade_idx = int(res.x + res.y + res.z);

	// [ world space -> light space ]
	vec4 pos_ls = ls_mats[cascade_idx] * vec4(pos, 1.0);
	vec3 proj_coords = pos_ls.xyz / pos_ls.w;
	proj_coords = proj_coords * 0.5 + 0.5;  // [ -1, 1 ] -> [ 0, 1 ]
	float curr_depth = proj_coords.z;

	float shadow = 0.0;
	vec2 tex_sz = 1.0 / vec2(textureSize(dir_shadow_maps, 0));
	float bias = max(0.05 * (1.0 - dot(normal, light.direction)), 0.005);
	bias *= 1 / (cascade_depths[cascade_idx] * 0.5f);

	// pcf
	for (int x = -1; x <= 1; ++x) {
		for (int y = -1; y <= 1; ++y) {
			float closest_depth = texture(dir_shadow_maps, vec3(proj_coords.xy + tex_sz * vec2(x, y), cascade_idx)).r;
			shadow += (curr_depth - bias) > closest_depth ? 1.0 : 0.0;
		}
	}

	shadow /= 9;
	return shadow;
}

vec3 calc_dir_light(DirLight light, vec3 pos, float frag_depth, vec3 normal, float spec_factor, float occlusion) {
	
	float ambient_strength = 0.1;
	float diffuse_strength = max(dot(-normalize(light.direction), normal), 0.0);
	float specular_strength = 0.0;

	if (diffuse_strength != 0.0) {
		vec3 view_dir = normalize(camera_pos - pos);
		vec3 half_dir = normalize(light.direction + view_dir);
		specular_strength = pow(max(dot(view_dir, half_dir), 0.0), 16);
	}

	float shadow = calc_dir_shadow(dir_light, pos, frag_depth, normal);
	return (ambient_strength * occlusion + (1.0 - shadow) * (diffuse_strength + spec_factor * specular_strength)) * light.color;
}

float calc_pt_shadow(vec3 pos, vec3 light_pos, samplerCube shadow_map, float far_plane) {
	vec3 frag_to_light = pos - light_pos;
	float closest = texture(shadow_map, frag_to_light).r * far_plane;
	float depth = length(frag_to_light);
	float bias = 0.05;
	float shadow = ((depth - bias) > closest) ? 1.0 : 0.0;
	return shadow;
}

vec3 calc_pt_light(PointLight light, vec3 light_pos, uint light_id, vec3 pos, vec3 normal, float spec_factor, samplerCube shadow_map, float occlusion) {
	// calculate attenuation
	float dist = length(light_pos - pos);
	float window = pow((max(1 - pow(dist / light.radius, 4), 0.0)), 2);
	float attenuation = window * (1.0 / (dist * dist + 0.001));

	// TODO: ambient component should probably be configurable
	float ambient_strength = 0.1;
	vec3 light_dir = normalize(light_pos - pos);
	float diffuse_strength = max(dot(light_dir, normal), 0.0);
	float specular_strength = 0.0;

	if (diffuse_strength != 0.0) {
		vec3 view_dir = normalize(camera_pos - pos);
		vec3 half_dir = normalize(light_dir + view_dir);
		specular_strength = pow(max(dot(view_dir, half_dir), 0.0), 16);
	}

	float shadow = (light_id == pt_caster_id) ? calc_pt_shadow(pos, light_pos, shadow_map, far_z) : 0.0; // TODO: refactor this fn to avoid this hacky check
	return (ambient_strength * occlusion + (1.0 - shadow) * (diffuse_strength + spec_factor * specular_strength)) * light.color.xyz * attenuation * light.intensity;
}

void main() {
	// retrive gbuf values
	vec4 gbuf_pos = texture(gbuf_pos, fs_in.tex_coords);
	vec3 pos = gbuf_pos.xyz;									// world space
	float pos_z_vs = gbuf_pos.w;								// view space z-coord

	vec3 norm = texture(gbuf_norms, fs_in.tex_coords).rgb;		// world space
	vec3 color = texture(gbuf_colors, fs_in.tex_coords).rgb;
	float spec_factor = texture(gbuf_colors, fs_in.tex_coords).w;

	float occlusion = (ao_enabled) ? texture(occlusion_tex, fs_in.tex_coords).r : 1.0f;

	// determine the cluster this fragment belongs in
	uint cluster_z = uint((log(abs(pos_z_vs) / near_z) * float(grid_sz.z)) / log(far_z / near_z));
	vec2 cluster_sz = screen_dims / grid_sz.xy;
	uvec3 cluster_coord = uvec3(gl_FragCoord.xy / cluster_sz, cluster_z);
	uint cluster_idx = cluster_coord.x + (cluster_coord.y * grid_sz.x) + (cluster_coord.z * grid_sz.x * grid_sz.y);

	// compute directional light contribution
	vec3 result = color * calc_dir_light(dir_light, pos, pos_z_vs, norm, spec_factor, occlusion);

	// compute contributions from point lights
	for (uint idx = 0; idx < clusters[cluster_idx].count; ++idx) {
		uint light_idx = clusters[cluster_idx].indices[idx];
		result += color * calc_pt_light(light_data[light_idx], light_positions[light_idx].xyz, light_ids[light_idx], pos, norm, spec_factor, pt_shadow_map, occlusion);
	}
	
	frag_color = vec4(result, 1.0);
}