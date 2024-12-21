#version 460 core

layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 tex_coords;

out vs_data {
	vec2 tex_coords;
} vs_out;

struct DirLight {
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

layout (std140, binding = 1) uniform globals {
	mat4 projection;
	mat4 view;
	vec3 camera_pos;
	DirLight dir_light;
};

void main() {
	gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);
	vs_out.tex_coords = tex_coords;
};