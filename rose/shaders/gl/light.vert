// =============================================================================
//   simple shader to render point lights
// =============================================================================

#version 460 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;

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

void main() {
	gl_Position = projection * view * model * vec4(pos, 1.0);
}