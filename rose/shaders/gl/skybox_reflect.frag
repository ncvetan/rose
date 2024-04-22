#version 460 core

in vec3 normal;
in vec3 pos;

out vec4 frag_color;

uniform samplerCube cube_map;

layout (std140, binding = 2) uniform globals
{
	mat4 projection;  // 64
	mat4 view;		  // 128
	vec3 camera_pos;  // 144
};

void main() {
	vec3 ref_vec = reflect(normalize(pos - camera_pos), normalize(normal));
	frag_color = vec4(texture(cube_map, ref_vec).rgb, 1.0);
}