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
uniform int mip_level;	// current mip level

float karis_avg(vec3 color)
{
    return 1.0f / (1.0f + dot(pow(color, vec3(1.0f / 2.2f)), vec3(0.2126f, 0.7152f, 0.0722f)));
}

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

	// apply a karis average to the first downsample to mitigate the 'firefly' effect
	if (mip_level == 0) {
		vec3 g0 = (s00 + s20 + s02 + s22) / 4.0f;
		vec3 g1 = (s20 + s40 + s22 + s42) / 4.0f;
		vec3 g2 = (s02 + s22 + s04 + s24) / 4.0f;
		vec3 g3 = (s22 + s42 + s24 + s44) / 4.0f;
		vec3 g4 = (s11 + s31 + s13 + s33) / 4.0f;
		float kw0 = karis_avg(g0);
		float kw1 = karis_avg(g1);
		float kw2 = karis_avg(g2);
		float kw3 = karis_avg(g3);
		float kw4 = karis_avg(g4);
		downsample = vec4((kw0 * g0 + kw1 * g1 + kw2 * g2 + kw3 * g3 + kw4 * g4) / (kw0 + kw1 + kw2 + kw3 + kw4 + 0.0001f), 1.0);
	}
	else {
		// apply weights
		vec3 ret = vec3(s22 * 0.125f);
		ret += (s00 + s40 + s04 + s44) * 0.03125f;
		ret += (s20 + s02 + s42 + s24) * 0.0625f;
		ret += (s11 + s31 + s13 + s33) * 0.125f;
		downsample = vec4(ret, 1.0f);
	}

	downsample = max(downsample, 0.0001f);
}