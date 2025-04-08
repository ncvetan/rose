// =============================================================================
//   used to render a skybox given its cubemap texture 
// =============================================================================

#version 460 core

in vec3 tex_dir; 

out vec4 frag_color;

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

uniform samplerCube cube_map;

void main() {
	frag_color = texture(cube_map, tex_dir) * vec4(dir_light.color, 1.0);
}