#version 460

in vec2 tex_coords;

in vs_data {
	vec2 tex_coords;
} fs_in;

out vec4 frag_color;

uniform sampler2D tex;
uniform float exposure;

void main() {
	
	const float gamma = 2.2;
	
	vec3 hdr_color = texture(tex, fs_in.tex_coords).rgb;

	// reinhard tone mapping
	vec3 mapped_color = hdr_color / (hdr_color + vec3(1.0));

	// gamma correction
	mapped_color = pow(mapped_color, vec3(1.0 / gamma));

	frag_color = vec4(mapped_color, 1.0);
}