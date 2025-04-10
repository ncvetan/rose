// =============================================================================
//   shader for passing through only colors above a brightness threshold
// =============================================================================

#version 460 core

out vec4 frag_color;

in vs_data {
	vec2 tex_coords;
} fs_in;

uniform sampler2D tex;
uniform float bloom_threshold;

float calc_brightness(vec3 color) {
	return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

void main() {

	vec4 color = texture(tex, fs_in.tex_coords);
	float brightness = calc_brightness(color.rgb);
	vec3 ret = vec3(0.0);

	if (brightness > bloom_threshold) {
		ret = color.rgb;
	}

	frag_color = vec4(ret, 1.0);
}