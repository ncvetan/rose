#version 460 core
layout (location = 0) in vec3 a_pos;

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

uniform mat4 model;

void main() {
	gl_Position = projection * view * model * vec4(a_pos, 1.0);
}