// =============================================================================
//   applies core lighting algorithms to forward rendered objects
//   this enables the ability to do things like rendering transparent objects
// =============================================================================

#version 460 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;
layout (location = 2) in vec3 tang;
layout (location = 3) in vec2 uv;

out vs_data {
	mat3  tbn;
	vec3  frag_pos_ws;		// world space
	vec3  normal;			// tangent space
	vec2  tex_coords;
	float frag_pos_z_vs;	// view space z coordinate, used for clustered shading
	float spec_factor;
} vs_out;

struct Material {
	sampler2D	diffuse_map;
	sampler2D	specular_map;
	sampler2D	normal_map;
	sampler2D	displace_map;
	float		shine;
	bool		has_diffuse_map;
	bool		has_normal_map;
	bool		has_specular_map;
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

uniform mat4 model;
uniform Material material;

void main() {
	// TODO: would much prefer to have a method for combining normal mapped
	// and non normal mapped codepaths
	mat3 normal_mat = mat3(transpose(inverse(mat3(model))));
	mat3 tbn = mat3(1.0);
	if (material.has_normal_map) {
		vec3 t = normalize(normal_mat * tang);
		vec3 n = normalize(normal_mat * norm);
		t = normalize(t - dot(t, n) * n);			// re-orthogonalize
		vec3 b = cross(n, t);
		tbn = mat3(t, b, n);
	}

	float spec_factor = 0.0;
	if (material.has_specular_map) {
		spec_factor = texture(material.specular_map, uv).r;
	}

	vs_out.tbn = tbn;
	vs_out.frag_pos_ws = vec3(model * vec4(pos, 1.0));
	vs_out.frag_pos_z_vs = vec4(view * vec4(vs_out.frag_pos_ws, 1.0)).z;
	vs_out.normal = normalize(normal_mat * norm);
	vs_out.tex_coords = uv;
	vs_out.spec_factor = spec_factor;

	gl_Position = projection * view * model * vec4(pos, 1.0);
}