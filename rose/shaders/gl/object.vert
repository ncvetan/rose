#version 460 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_tex_coords;

out vs_data {
	vec3 frag_pos;
	vec3 normal;
	vec2 tex_coords;
} vs_out;

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
	vs_out.frag_pos = vec3(model * vec4(a_pos, 1.0));
	vs_out.normal = mat3(transpose(inverse(model))) * a_normal;
	vs_out.tex_coords = a_tex_coords;
	gl_Position = projection * view * vec4(vs_out.frag_pos, 1.0);
};