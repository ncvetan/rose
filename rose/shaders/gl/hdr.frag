#version 460

in vec2 tex_coords;

in vs_data {
	vec2 tex_coords;
} fs_in;

out vec4 frag_color;

uniform sampler2D color_buf;
uniform float exposure;
uniform float gamma;

void main() {
	
	vec3 hdr_color = texture(color_buf, fs_in.tex_coords).rgb;

	// tone mapping
	vec3 mapped_color = vec3(1.0) - exp(-hdr_color * exposure);
	
	// gamma correction
	mapped_color = pow(mapped_color, vec3(1.0 / gamma));
	
	frag_color = vec4(mapped_color, 1.0);
}