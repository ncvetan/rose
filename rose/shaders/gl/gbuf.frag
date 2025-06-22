// =============================================================================
//   shader for filling out gbuffers
// =============================================================================

#version 460 core

// gbuffer layout:
//
// [     R     ] [     G     ] [     B     ] [     A     ]	
// [                Position               ] [   VS Z    ]
// [                 Normal                ] [ Roughness ]
// [                 Color                 ] [    AO     ]
// [ Metalness ]

layout (location = 0) out vec4  gbuf_pos;
layout (location = 1) out vec4  gbuf_norm;
layout (location = 2) out vec4  gbuf_color;
layout (location = 3) out float gbuf_metallic;

in vs_data {
	mat3  tbn;
	vec3  frag_pos_ws;		// world space
	vec3  normal;			// tangent space
	vec2  tex_coords;
	float frag_pos_z_vs;	// view space
} fs_in;

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

uniform Material material;

void main() {

	vec3 norm = (material.has_normal_map) ? fs_in.tbn * (texture(material.normal_map, fs_in.tex_coords).rgb * 2.0f - 1.0f) : fs_in.normal;
	
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
	
	// TODO: reimplement displacement mapping
	gbuf_pos.rgb = fs_in.frag_pos_ws;
	gbuf_pos.a = fs_in.frag_pos_z_vs;
	
	gbuf_norm.rgb = normalize(norm);
	gbuf_norm.a = roughness;
	
	// [ sRGB -> Linear ]
	gbuf_color.rgb = (material.has_albedo_map) ? pow(texture(material.albedo_map, fs_in.tex_coords).rgb, vec3(2.2f, 2.2f, 2.2f)) : vec3(0.5f, 0.5f, 0.5f);
	gbuf_color.a = ambient_occ;

	gbuf_metallic = metallic;
}