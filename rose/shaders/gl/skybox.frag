#version 460 core

in vec3 tex_dir; 

out vec4 frag_color;

uniform samplerCube cube_map;

void main() {
	frag_color = texture(cube_map, tex_dir);
}