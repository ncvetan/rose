// =============================================================================
//   shader for computing bloom effect using a gaussian blur
// =============================================================================

#version 460 core

out vec4 frag_color;

in vs_data {
	vec2 tex_coords;
} fs_in;

uniform sampler2D tex;
uniform bool horizontal;
uniform float bloom_threshold;

const float weights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };

float calc_brightness(vec3 color) {
	return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

void main() {

	vec4 color = texture(tex, fs_in.tex_coords);
	vec2 tex_sz = 1.0 / textureSize(tex, 0);
	
	vec3 ret = vec3(0.0);
	float tot_weight = 0.0;

	float brightness = calc_brightness(color.rgb);
	if (brightness > bloom_threshold) {
		ret += color.rgb * weights[0];
		tot_weight += weights[0];
	}

	// two pass gaussian blur
	if (horizontal) {
		for(int i = 1; i < 5; ++i) {
			vec3 sample_l = texture(tex, fs_in.tex_coords - vec2(tex_sz.x * i, 0.0)).rgb;
			vec3 sample_r = texture(tex, fs_in.tex_coords + vec2(tex_sz.x * i, 0.0)).rgb;
			ret += sample_l * weights[i];
			ret += sample_r * weights[i];
			tot_weight += weights[i];
			tot_weight += weights[i];
		}
	}
	else {
		for(int i = 1; i < 5; ++i) {
			vec3 sample_t = texture(tex, fs_in.tex_coords - vec2(0.0, tex_sz.y * i)).rgb;
			vec3 sample_b = texture(tex, fs_in.tex_coords + vec2(0.0, tex_sz.y * i)).rgb;
			ret += sample_t * weights[i];
			ret += sample_b * weights[i];
			tot_weight += weights[i];
			tot_weight += weights[i];
		}
	}

	ret = (tot_weight > 0.0) ? (ret / tot_weight) : vec3(0.0);
	frag_color = vec4(ret, 1.0);
}