#version 460 core

layout (location = 0) in vec3 a_pos;

out vec3 tex_dir;

struct DirLight {
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

layout (std140, binding = 2) uniform globals
{
	mat4 projection;     // 64
	mat4 view;		     // 128
	vec3 camera_pos;     // 144
	DirLight dir_light;  // 208
};

void main() {
    tex_dir = a_pos;    
    gl_Position = projection * view * vec4(a_pos, 1.0);
}