#version 460 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_tex_coords;

uniform mat4 model;

void main() {
	gl_Position = model * vec4(a_pos, 1.0);
}