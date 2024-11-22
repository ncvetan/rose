#version 460 core

layout (location = 0) out vec4 frag_color;
layout (location = 1) out vec4 brightness_color;

void main() {
	frag_color = vec4(100.0, 100.0, 100.0, 1.0);
	brightness_color = vec4(1.0, 1.0, 1.0, 1.0);
}