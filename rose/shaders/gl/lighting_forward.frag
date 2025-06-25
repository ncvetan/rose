// =============================================================================
//   applies core lighting algorithms to forward rendered objects
//   this enables the ability to do things like rendering transparent objects
// =============================================================================

#version 460 core

layout (location = 0) out vec4 frag_color;

// inputs =========================================================================================

in vs_data {
	mat3  tbn;
	vec3  frag_pos_ws;		// world space
	vec3  normal;			// tangent space
	vec2  tex_coords;
	float frag_pos_z_vs;	// view space z coordinate, used for clustered shading
} fs_in;

// struct definitions =============================================================================

// directional light properties
struct DirLight {
	vec3 direction;
	vec3 color;
	float ambient_strength;
};

struct Material {
	sampler2D	albedo_map;
	sampler2D	normal_map;
	sampler2D	displace_map;
	sampler2D   pbr_map;
	sampler2D   ao_map;
	bool		has_albedo_map;
	bool		has_normal_map;
	bool		has_pbr_map;
	bool		has_ao_map;
};

// light parameters for a particular point light
struct PointLight {
    vec4 color;
    float radius;
    float intensity;
};

// uniforms =======================================================================================

uniform sampler2DArray dir_shadow_maps;	 // shadow map for each cascade
uniform DirLight dir_light;				 // directional light properties
uniform Material material;				 // material properties
uniform int n_cascades;					 // number of shadow cascades
uniform float cascade_depths[3];		 // far depth of each shadow cascade
uniform samplerCube pt_shadow_map;		 // shadow map for point lights
uniform uint pt_caster_id;				 // id of the current shadow casting point light

layout (std140, binding = 1) uniform globals_ubo {
	mat4 projection;
	mat4 view;
	vec3 camera_pos;
	uvec3 grid_sz;				// cluster dimensions (xyz)
	uvec2 screen_dims;			// screen [ width, height ]
	float far_z;
	float near_z;
};

const float pi = 3.14159265359f;

// buffers ========================================================================================

// global list of lights and their parameters
layout (std430, binding=3) buffer lights_ssbo {
    PointLight light_data[];
};

// global list of light positions (this should always have the same length as lights_ssbo)
layout (std430, binding=4) buffer light_positions_ssbo {
    vec4 light_positions[];
};

struct Cluster {
    uint count;			// number of lights affecting the cluster
    uint indices[100];	// indicies of lights up to a maximum of 100
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
	vec3 F0 = vec3(0.04f);	// default for dielectrics
	F0 = mix(F0, color, metalness);
    return F0 + (1.0f - F0) * pow(clamp(1.0f - angle, 0.0f, 1.0f), 5.0f);
}

// computes the distribution of microfacet orientations using the GGX model
float distribution(vec3 norm, vec3 halfway, float roughness) {
    float a2   = pow(roughness, 4);
    float ndh  = max(dot(norm, halfway), 0.0f);
    float ndh2 = ndh * ndh;
    return a2 / (pi * pow((ndh2 * (a2 - 1.0f) + 1.0f), 2));
}

float geo_shlick_ggx(float ndv, float roughness) {
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;
    return ndv / (ndv * (1.0f - k) + k);
}

// geometry function for computing the probability that a microfacet is visible from the 
// view direction and light direction using the Smith model
float geometry(vec3 norm, vec3 view_dir, vec3 light_dir, float roughness) {
	float ndv = max(dot(norm, view_dir), 0.0f);
	float ndl = max(dot(norm, light_dir), 0.0f);
	float light_shadowing = geo_shlick_ggx(ndl, roughness);
	float view_shadowing  = geo_shlick_ggx(ndv, roughness);
	return light_shadowing * view_shadowing;
}

float calc_dir_shadow(vec3 pos, float frag_depth, vec3 normal) {

	vec3 res = step(vec3(cascade_depths[0], cascade_depths[1], cascade_depths[2]), vec3(abs(frag_depth)));
	int cascade_idx = int(res.x + res.y + res.z);

	// [ world space -> light space ]
	vec4 pos_ls = ls_mats[cascade_idx] * vec4(pos, 1.0f);
	vec3 proj_coords = pos_ls.xyz / pos_ls.w;
	proj_coords = proj_coords * 0.5f + 0.5f;  // [ -1, 1 ] -> [ 0, 1 ]
	float curr_depth = proj_coords.z;

	float shadow = 0.0f;
	vec2 tex_sz = 1.0f / vec2(textureSize(dir_shadow_maps, 0));
	float bias = max(0.05f * (1.0f - dot(normal, dir_light.direction)), 0.005f);
	bias *= 1 / (cascade_depths[cascade_idx] * 0.5f);

	// pcf
	for (int x = -1; x <= 1; ++x) {
		for (int y = -1; y <= 1; ++y) {
			float closest_depth = texture(dir_shadow_maps, vec3(proj_coords.xy + tex_sz * vec2(x, y), cascade_idx)).r;
			shadow += (curr_depth - bias) > closest_depth ? 1.0f : 0.0f;
		}
	}

	shadow /= 9;
	return shadow;
}

float calc_pt_shadow(vec3 pos, vec3 light_pos, samplerCube shadow_map, float far_plane) {
	vec3 frag_to_light = pos - light_pos;
	float closest = texture(shadow_map, frag_to_light).r * far_plane;
	float depth = length(frag_to_light);
	float bias = 0.05f;
	float shadow = ((depth - bias) > closest) ? 1.0f : 0.0f;
	return shadow;
}

float calc_attenuation(float dist, float radius) {
	float window = pow((max(1 - pow(dist / radius, 4), 0.0f)), 2);
	return window * (1.0f / (dist * dist + 0.001f));
}

vec3 calc_dir_light(vec3 frag_pos, float frag_depth, vec3 normal, vec3 albedo, float metallic, float roughness, float ao) {
	
	vec3 light_dir = -dir_light.direction;
	vec3 view_dir = normalize(camera_pos - frag_pos);
	vec3 half_dir = normalize(light_dir + view_dir);
	float ndotl = max(dot(normal, light_dir), 0.0f);
	
	float ndf = distribution(normal, half_dir, roughness);
	float geo = geometry(normal, view_dir, light_dir, roughness);
	vec3 fres = fresnel(dir_light.color.xyz, metallic, max(dot(half_dir, view_dir), 0.0f));
	vec3 specular = (ndf * geo * fres) / (4.0f * max(dot(normal, view_dir), 0.0f) * ndotl + 0.0001f);
	vec3 kd = (vec3(1.0f) - fres) * (1.0f - metallic);
	vec3 radiance_out = (kd * albedo / pi + specular) * (dir_light.color.rgb) * ndotl;
	float shadow = calc_dir_shadow(frag_pos, frag_depth, normal);

	return (1.0f - shadow) * radiance_out;
}

vec3 calc_pt_light(PointLight light, vec3 light_pos, uint light_id, vec3 frag_pos, vec3 normal, vec3 albedo, samplerCube shadow_map, float metallic, float roughness) {
	
	float attenuation = calc_attenuation(length(light_pos - frag_pos), light.radius);

	vec3 light_dir = normalize(light_pos - frag_pos);
	vec3 view_dir = normalize(camera_pos - frag_pos);
	vec3 half_dir = normalize(light_dir + view_dir);
	float ndl = max(dot(normal, light_dir), 0.0);
	
	float ndf = distribution(normal, half_dir, roughness);
	float geo = geometry(normal, view_dir, light_dir, roughness);
	vec3 fres = fresnel(light.color.xyz, metallic, max(dot(half_dir, view_dir), 0.0f));
	vec3 specular = (ndf * geo * fres) / (4.0f * max(dot(normal, view_dir), 0.0f) * ndl + 0.0001f);

	vec3 kd = (vec3(1.0) - fres) * (1.0f - metallic);
	vec3 radiance_out = (kd * albedo / pi + specular) * light.color.rgb * attenuation * ndl;

	float shadow = (light_id == pt_caster_id) ? calc_pt_shadow(frag_pos, light_pos, shadow_map, far_z) : 0.0f;
	return (1.0 - shadow) * radiance_out * light.intensity;
}

void main() {

	vec4 albedo = (material.has_albedo_map) ? pow(texture(material.albedo_map, fs_in.tex_coords), vec4(2.2f, 2.2f, 2.2f, 1.0f)) : vec4(0.5f, 0.5f, 0.5f, 1.0f);
	vec3 norm = (material.has_normal_map) ? fs_in.tbn * (texture(material.normal_map, fs_in.tex_coords).rgb * 2.0f - 1.0f) : fs_in.normal;
	norm = normalize(norm);

	float roughness = 1.0f;
	float ambient_occ = 1.0f;
	float metallic = 0.0f;

	if (material.has_pbr_map) {
		vec3 pbr = texture(material.pbr_map, fs_in.tex_coords).rgb;
		roughness = pbr.g;
		metallic = pbr.b;
	}

	if (material.has_ao_map) { 
		ambient_occ = texture(material.ao_map, fs_in.tex_coords).r;
	}

	// discard fragments with low alpha
	if (albedo.a < 0.1f) {
		discard;
	}
	
	// determine the cluster this fragment belongs in
	uint cluster_z = uint((log(abs(fs_in.frag_pos_z_vs) / near_z) * float(grid_sz.z)) / log(far_z / near_z));
	vec2 cluster_sz = screen_dims / grid_sz.xy;
	uvec3 cluster_coord = uvec3(gl_FragCoord.xy / cluster_sz, cluster_z);
	uint cluster_idx = cluster_coord.x + (cluster_coord.y * grid_sz.x) + (cluster_coord.z * grid_sz.x * grid_sz.y);

	// compute directional light contribution
	vec3 result = calc_dir_light(fs_in.frag_pos_ws, fs_in.frag_pos_z_vs, norm, albedo.rgb, metallic, roughness, ambient_occ);

	// compute contributions from point lights
	for (uint idx = 0; idx < clusters[cluster_idx].count; ++idx) {
		uint light_idx = clusters[cluster_idx].indices[idx];
		result += calc_pt_light(light_data[light_idx], light_positions[light_idx].xyz, light_ids[light_idx], fs_in.frag_pos_ws, norm, albedo.rgb, pt_shadow_map, metallic, roughness);
	}
	
	// add ambient component
	vec3 ambient = dir_light.ambient_strength * ambient_occ * albedo.rgb;
	result += ambient;

	frag_color = vec4(result, 1.0f);
}