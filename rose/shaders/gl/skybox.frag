#version 460 core

in vec3 tex_dir; 

out vec4 frag_color;

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

uniform samplerCube cube_map;

void main() {
	frag_color = texture(cube_map, tex_dir) * vec4(dir_light.ambient, 1.0);
}