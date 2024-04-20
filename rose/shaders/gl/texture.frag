#version 460 core

in vec3 frag_pos;
in vec2 tex_coords;

uniform sampler2D tex;
out vec4 frag_color;

void main() {
	vec4 col = texture(tex, tex_coords);
	frag_color = col;
}