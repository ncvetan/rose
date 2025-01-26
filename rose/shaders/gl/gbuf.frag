#version 460 core

layout (location = 0) out vec4 gbuf_pos;
layout (location = 1) out vec3 gbuf_norm;
layout (location = 2) out vec4 gbuf_color;

in vs_data {
	vec3 frag_pos_ws;		// world space
	vec3 frag_pos_ts;		// tangent space
	float frag_pos_z_vs;	// view space
	vec3 normal;			// tangent space
	vec3 view_pos_ts;		// tangent space
	vec2 tex_coords;
} fs_in;

struct Material {
	sampler2D	diffuse_map;
	sampler2D	specular_map;
	sampler2D	normal_map;
	sampler2D	displace_map;
	float		shine;
};

uniform Material material;

void main() {
	// write positions, normals, colors, and specular instensities to the g-buffer
	gbuf_pos.xyz = fs_in.frag_pos_ws;
	gbuf_pos.w = fs_in.frag_pos_z_vs;
	gbuf_norm = normalize(fs_in.normal);
	gbuf_color.rgb = texture(material.diffuse_map, fs_in.tex_coords).rgb;
	gbuf_color.a = texture(material.specular_map, fs_in.tex_coords).r;	
}