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
	float ambient_strength;
};

layout (std140, binding = 1) uniform globals_ubo {
	mat4 projection;
	mat4 view;
	vec3 camera_pos;
	uvec3 grid_sz;				// cluster dimensions (xyz)
	uvec2 screen_dims;			// screen [ width, height ]
	float far_z;
	float near_z;
};

const float pi = 3.14159265359;

// uniforms =======================================================================================

// g-buffers
uniform sampler2D gbuf_pos;				 // xyz = world space pos,  w = view space z 
uniform sampler2D gbuf_norms;			 // xyz = world space norm, w = roughness
uniform sampler2D gbuf_colors;			 // xyz = albedo,			w = ambient occlusion
uniform sampler2D gbuf_metallic;		 // x = metallic

uniform DirLight dir_light;				 // directional light properties
uniform sampler2DArray dir_shadow_maps;	 // shadow map for each cascade

uniform int n_cascades;					 // number of shadow cascades
uniform float cascade_depths[3];		 // far depth of each shadow cascade
uniform samplerCube pt_shadow_map;		 // shadow map for point lights
uniform uint pt_caster_id;				 // id of the current shadow casting point light
uniform bool ssao_enabled;				 // indicates whether ambient occlusion is enabled	
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

// computes fraction of incoming light that is reflected as opposed to refracted
// for a given lighting angle between the light and halfway vectors
// uses the Schlick approximation
vec3 fresnel(vec3 color, float metalness, float angle)
{
	vec3 F0 = vec3(0.04);	// default for dielectrics
	F0 = mix(F0, color, metalness);
    return F0 + (1.0 - F0) * pow(clamp(1.0 - angle, 0.0, 1.0), 5.0);
}

// computes the distribution of microfacet orientations using the GGX model
float distribution(vec3 norm, vec3 halfway, float roughness) {
    float a2   = pow(roughness, 4);
    float ndh  = max(dot(norm, halfway), 0.0);
    float ndh2 = ndh * ndh;
    return a2 / (pi * pow((ndh2 * (a2 - 1.0) + 1.0), 2));
}

float geo_shlick_ggx(float ndv, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return ndv / (ndv * (1.0 - k) + k);
}

// geometry function for computing the probability that a microfacet is visible from the 
// view direction and light direction using the Smith model
float geometry(vec3 norm, vec3 view_dir, vec3 light_dir, float roughness) {
	float ndv = max(dot(norm, view_dir), 0.0);
	float ndl = max(dot(norm, light_dir), 0.0);
	float light_shadowing = geo_shlick_ggx(ndl, roughness);
	float view_shadowing  = geo_shlick_ggx(ndv, roughness);
	return light_shadowing * view_shadowing;
}

float calc_dir_shadow(vec3 frag_pos, float frag_depth, vec3 normal) {

	vec3 res = step(vec3(cascade_depths[0], cascade_depths[1], cascade_depths[2]), vec3(abs(frag_depth)));
	int cascade_idx = int(res.x + res.y + res.z);

	// [ world space -> light space ]
	vec4 frag_pos_ls = ls_mats[cascade_idx] * vec4(frag_pos, 1.0);
	vec3 proj_coords = frag_pos_ls.xyz / frag_pos_ls.w;
	proj_coords = proj_coords * 0.5 + 0.5;  // [ -1, 1 ] -> [ 0, 1 ]
	float curr_depth = proj_coords.z;

	float shadow = 0.0;
	vec2 tex_sz = 1.0 / vec2(textureSize(dir_shadow_maps, 0));
	float bias = max(0.05 * (1.0 - dot(normal, dir_light.direction)), 0.005);
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

float calc_pt_shadow(vec3 frag_pos, vec3 light_pos, samplerCube shadow_map, float far_plane) {
	vec3 frag_to_light = frag_pos - light_pos;
	float closest = texture(shadow_map, frag_to_light).r * far_plane;
	float depth = length(frag_to_light);
	float bias = 0.05;
	float shadow = ((depth - bias) > closest) ? 1.0 : 0.0;
	return shadow;
}

float calc_attenuation(float dist, float radius) {
	float window = pow((max(1 - pow(dist / radius, 4), 0.0)), 2);
	return window * (1.0 / (dist * dist + 0.001));
}

vec3 calc_dir_light(vec3 frag_pos, float frag_depth, vec3 normal, vec3 albedo, float roughness, float metallic) {
	
	vec3 light_dir = -dir_light.direction;
	vec3 view_dir = normalize(camera_pos - frag_pos);
	vec3 half_dir = normalize(light_dir + view_dir);
	float ndotl = max(dot(normal, light_dir), 0.0);
	
	float ndf = distribution(normal, half_dir, roughness);
	float geo = geometry(normal, view_dir, light_dir, roughness);
	vec3 fres = fresnel(dir_light.color.xyz, metallic, max(dot(half_dir, view_dir), 0.0));
	vec3 specular = (ndf * geo * fres) / (4.0 * max(dot(normal, view_dir), 0.0) * ndotl + 0.0001);

	vec3 kd = (vec3(1.0) - fres) * (1.0 - metallic);
	vec3 radiance_out = (kd * albedo / pi + specular) * (dir_light.color.rgb) * ndotl;

	float shadow = calc_dir_shadow(frag_pos, frag_depth, normal);

	return (1.0 - shadow) * radiance_out;
}

vec3 calc_pt_light(PointLight light, vec3 light_pos, uint light_id, vec3 frag_pos, vec3 albedo, vec3 normal, float roughness, float metallic, samplerCube shadow_map) {

	float attenuation = calc_attenuation(length(light_pos - frag_pos), light.radius);

	vec3 light_dir = normalize(light_pos - frag_pos);
	vec3 view_dir = normalize(camera_pos - frag_pos);
	vec3 half_dir = normalize(light_dir + view_dir);
	float ndl = max(dot(normal, light_dir), 0.0);
	
	float ndf = distribution(normal, half_dir, roughness);
	float geo = geometry(normal, view_dir, light_dir, roughness);
	vec3 fres = fresnel(light.color.xyz, metallic, max(dot(half_dir, view_dir), 0.0));
	vec3 specular = (ndf * geo * fres) / (4.0 * max(dot(normal, view_dir), 0.0) * ndl + 0.0001);

	vec3 kd = (vec3(1.0) - fres) * (1.0 - metallic);
	vec3 radiance_out = (kd * albedo / pi + specular) * (light.color.rgb * attenuation) * ndl;

	float shadow = (light_id == pt_caster_id) ? calc_pt_shadow(frag_pos, light_pos, shadow_map, far_z) : 0.0;
	return (1.0 - shadow) * radiance_out * light.intensity;
}

void main() {
	
	// retrive parameters
	vec4 frag_gbuf_pos = texture(gbuf_pos, fs_in.tex_coords);
	vec4 frag_gbuf_norm = texture(gbuf_norms, fs_in.tex_coords);
	vec4 frag_gbuf_albedo = texture(gbuf_colors, fs_in.tex_coords);
	float metallic = texture(gbuf_metallic, fs_in.tex_coords).r;

	vec3 frag_pos = frag_gbuf_pos.xyz;
	float frag_pos_z_vs = frag_gbuf_pos.w;
	vec3 norm = frag_gbuf_norm.rgb;
	float roughness = frag_gbuf_norm.a;
	vec3 albedo = frag_gbuf_albedo.rgb;
	float occlusion = frag_gbuf_albedo.a;

	float ssao = (ssao_enabled) ? texture(occlusion_tex, fs_in.tex_coords).r : 1.0f;

	// determine the cluster this fragment belongs in
	uint cluster_z = uint((log(abs(frag_pos_z_vs) / near_z) * float(grid_sz.z)) / log(far_z / near_z));
	vec2 cluster_sz = screen_dims / grid_sz.xy;
	uvec3 cluster_coord = uvec3(gl_FragCoord.xy / cluster_sz, cluster_z);
	uint cluster_idx = cluster_coord.x + (cluster_coord.y * grid_sz.x) + (cluster_coord.z * grid_sz.x * grid_sz.y);

	// compute directional light contribution
	vec3 result = calc_dir_light(frag_pos, frag_pos_z_vs, norm, albedo, roughness, metallic);

	// compute contributions from point lights
	for (uint idx = 0; idx < clusters[cluster_idx].count; ++idx) {
		uint light_idx = clusters[cluster_idx].indices[idx];
		result += calc_pt_light(light_data[light_idx], light_positions[light_idx].xyz, light_ids[light_idx], frag_pos, albedo, norm, roughness, metallic, pt_shadow_map);
	}
	
	// add ambient component
	vec3 ambient = dir_light.ambient_strength * ssao * occlusion * albedo;
	result += ambient;

	frag_color = vec4(result, 1.0);
}