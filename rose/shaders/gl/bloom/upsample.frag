// =============================================================================
//   shader for upsampling and blurring the given texture
// =============================================================================

// upsampling filter courtesy of Sledgehammer Games
// see: https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/

#version 460 core

out vec4 upsample;

in vs_data {
	vec2 tex_coords;
} fs_in;

uniform sampler2D tex;		// texture being sampled
uniform vec2 filter_sz;		// radius of the filter

void main() {
	
	float u = fs_in.tex_coords.x;
	float v = fs_in.tex_coords.y;
	float x = filter_sz.x;
	float y = filter_sz.y;

	// 3x3 filter
	vec3 s00 = texture(tex, vec2(u - x, v - y)).rgb;
	vec3 s10 = texture(tex, vec2(u,     v - y)).rgb;
	vec3 s20 = texture(tex, vec2(u + x, v - y)).rgb;

	vec3 s01 = texture(tex, vec2(u - x, v)).rgb;
	vec3 s11 = texture(tex, vec2(u,     v)).rgb;
	vec3 s21 = texture(tex, vec2(u + x, v)).rgb;

	vec3 s02 = texture(tex, vec2(u - x, v + y)).rgb;
	vec3 s12 = texture(tex, vec2(u,     v + y)).rgb;
	vec3 s22 = texture(tex, vec2(u + x, v + y)).rgb;

	// apply weights
	vec3 ret = s11 * 4.0;
	ret += (s10 + s01 + s21 + s12) * 2.0;
	ret += (s00 + s20 + s02 + s22) * 1.0;
	ret /= 16.0;  // normalize
	upsample = vec4(ret, 1.0);
}