// =============================================================================
//   used to render a skybox given its cubemap texture 
// =============================================================================

#version 460 core

layout (location = 0) in vec3 a_pos;

out vec3 tex_dir;

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

uniform mat4 static_view;

void main() {
    tex_dir = a_pos;    
    gl_Position = projection * static_view * vec4(a_pos, 1.0);
}