#version 460 core

layout (location = 0) out vec4 frag_color;
layout (location = 1) out vec4 brightness_color;

in vs_data {
	vec2 tex_coords;
} fs_in;

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

uniform sampler2D gbuf_colors;

void main() {
	vec3 color = texture(gbuf_colors, fs_in.tex_coords).rgb;
	frag_color = vec4(color, 1.0);
	
	float brightness = dot(frag_color.rgb, vec3(0.2126, 0.7152, 0.0722));	// rgb -> luminance

	if (brightness > 1.0) {
		brightness_color = vec4(frag_color.rgb, 0.0);
	}
	else {
		brightness_color = vec4(0.0, 0.0, 0.0, 0.0);
	}
}