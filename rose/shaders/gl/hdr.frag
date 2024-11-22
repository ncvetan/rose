#version 460

in vs_data {
	vec2 tex_coords;
} fs_in;

out vec4 frag_color;

uniform sampler2D scene_tex;
uniform sampler2D blur_tex;
uniform float gamma;
uniform float exposure;
uniform bool bloom_enabled;

void main() {
	
	vec3 hdr_color = texture(scene_tex, fs_in.tex_coords).rgb;

	if (bloom_enabled) {
		vec3 bloom_color = texture(blur_tex, fs_in.tex_coords).rgb;
		hdr_color += bloom_color;
	}

	// tone mapping
	vec3 mapped_color = vec3(1.0) - exp(-hdr_color.rgb * exposure);
	
	// gamma correction
	mapped_color = pow(mapped_color, vec3(1.0 / gamma));
	
	frag_color = vec4(mapped_color, 1.0);
}