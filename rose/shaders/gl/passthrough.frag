// =============================================================================
//   used to passthrough colors from the gbuf onto the post-processing stages
// =============================================================================

#version 460 core

layout (location = 0) out vec4 frag_color;

in vs_data {
	vec2 tex_coords;
} fs_in;

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

uniform sampler2D gbuf_colors;

void main() {
	vec3 color = texture(gbuf_colors, fs_in.tex_coords).rgb;
	frag_color = vec4(color, 1.0);
}