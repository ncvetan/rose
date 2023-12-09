#version 400 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_tex_coords;

out vec3 frag_pos;
out vec3 normal;
out vec2 tex_coords;
out vec3 light_pos_view;

uniform vec3 light_pos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
	frag_pos = vec3(view * model * vec4(a_pos, 1.0));
	normal = mat3(transpose(inverse(view * model))) * a_normal;
	light_pos_view = vec3(view * vec4(light_pos, 1.0));
	tex_coords = a_tex_coords;
	
	gl_Position = projection * view * model * vec4(a_pos, 1.0);
};