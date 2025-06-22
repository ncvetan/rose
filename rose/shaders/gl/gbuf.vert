// =============================================================================
//   shader for filling out gbuffers
// =============================================================================

#version 460 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec2 tex_coords;

out vs_data {
	mat3  tbn;
	vec3  frag_pos_ws;		// world space
	vec3  normal;			// tangent space
	vec2  tex_coords;
	float frag_pos_z_vs;	// view space z coordinate, used for clustered shading
} vs_out;

layout (std140, binding = 1) uniform globals_ubo {
	mat4 projection;
	mat4 view;
	vec3 camera_pos;
	uvec3 grid_sz;				// cluster dimensions (xyz)
	uvec2 screen_dims;			// screen [ width, height ]
	float far_z;
	float near_z;
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

uniform mat4 model;
uniform Material material;

void main() {
	
	mat3 normal_mat = mat3(transpose(inverse(mat3(model))));
	mat3 tbn = mat3(1.0);

	// TODO: would much prefer to have a method for combining normal mapped
	// and non normal mapped codepaths
	if (material.has_normal_map) {
		vec3 t = normalize(normal_mat * tangent);
		vec3 n = normalize(normal_mat * normal);
		t = normalize(t - dot(t, n) * n);			// re-orthogonalize
		vec3 b = cross(n, t);
		tbn = mat3(t, b, n);
	}

	vs_out.tbn = tbn;
	vs_out.frag_pos_ws = vec3(model * vec4(pos, 1.0));
	vs_out.frag_pos_z_vs = vec4(view * vec4(vs_out.frag_pos_ws, 1.0)).z;
	vs_out.normal = normal_mat * normal;
	vs_out.tex_coords = tex_coords;

	gl_Position = projection * view * model * vec4(pos, 1.0);
};