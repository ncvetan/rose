#version 460 core

out vec4 frag_color;

in vs_data {
	vec2 tex_coords;
} fs_in;

uniform sampler2D tex;
uniform bool horizontal;

void main() {

	// gaussian blur weights
	const float weight[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };

	vec2 tex_offset = 1.0 / textureSize(tex, 0);					// size of single texel
	vec3 result = texture(tex, fs_in.tex_coords).rgb * weight[0];	// this fragment
	
	if (horizontal) {
		for(int i = 1; i < 5; ++i) {
			result += texture(tex, fs_in.tex_coords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
			result += texture(tex, fs_in.tex_coords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
		}
	}
	else {
		for(int i = 1; i < 5; ++i) {
			result += texture(tex, fs_in.tex_coords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
			result += texture(tex, fs_in.tex_coords - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
		}
	}
	
	frag_color = vec4(result, 1.0); // set this back to the result
}