#version 460 core

layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 tex_coords;

out vs_data {
	vec2 tex_coords;
} vs_out;

void main() {
	gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);
	vs_out.tex_coords = tex_coords;
};