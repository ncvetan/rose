#version 460 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_tex;

out vec3 normal;
out vec3 pos;

uniform mat4 model;

layout (std140, binding = 2) uniform globals
{
	mat4 projection;  // 64
	mat4 view;		  // 128
	vec3 camera_pos;  // 144
};

void main() {
    normal = mat3(transpose(inverse(model))) * a_normal;
	pos = vec3(model * vec4(a_pos, 1.0));
    gl_Position = projection * view * vec4(pos, 1.0);
}