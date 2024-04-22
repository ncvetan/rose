#version 460 core

layout (location = 0) in vec3 a_pos;

out vec3 tex_dir;

layout (std140, binding = 2) uniform globals
{
	mat4 projection;  // 64
	mat4 view;		  // 128
	vec3 camera_pos;  // 144
};

void main() {
    tex_dir = a_pos;    
    gl_Position = projection * view * vec4(a_pos, 1.0);
}