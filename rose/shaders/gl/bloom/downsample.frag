// =============================================================================
//   shader for downsampling the given texture
// =============================================================================

// downsampling filter courtesy of Sledgehammer Games
// see: https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/

#version 460 core

out vec4 downsample;

in vs_data {
	vec2 tex_coords;
} fs_in;

uniform sampler2D tex;	// texture being sampled
uniform vec2 texel_sz;	// dimensions of a texel for 'tex'

void main() {

	float u = fs_in.tex_coords.x;
	float v = fs_in.tex_coords.y;
	float x = texel_sz.x;
	float y = texel_sz.y;

	vec3 s00 = texture(tex, vec2(u - 2 * x, v + 2 * y)).rgb;
	vec3 s20 = texture(tex, vec2(u,			v + 2 * y)).rgb;
	vec3 s40 = texture(tex, vec2(u + 2 * x, v + 2 * y)).rgb;

	vec3 s11 = texture(tex, vec2(u - x, v + y)).rgb;
	vec3 s31 = texture(tex, vec2(u + x, v + y)).rgb;

	vec3 s02 = texture(tex, vec2(u - 2 * x, v)).rgb;
	vec3 s22 = texture(tex, vec2(u,			v)).rgb;
	vec3 s42 = texture(tex, vec2(u + 2 * x, v)).rgb;

	vec3 s13 = texture(tex, vec2(u - x, v - y)).rgb;
	vec3 s33 = texture(tex, vec2(u + x, v - y)).rgb;

	vec3 s04 = texture(tex, vec2(u - 2 * x,	v - 2 * y)).rgb;
	vec3 s24 = texture(tex, vec2(u,			v - 2 * y)).rgb;
	vec3 s44 = texture(tex, vec2(u + 2 * x, v - 2 * y)).rgb;

	// apply weights
	vec3 ret = vec3(s22 * 0.215);
	ret += (s00 + s40 + s04 + s44) * 0.03125;
	ret += (s20 + s02 + s42 + s24) * 0.0625;
	ret += (s11 + s31 + s13 + s33) * 0.125;
	downsample = vec4(ret, 1.0);
}