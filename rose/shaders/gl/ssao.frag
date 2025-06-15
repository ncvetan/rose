// =============================================================================
//   shader for computing SSAO effect
// =============================================================================

#version 460

in vs_data {
	vec2 tex_coords;
} fs_in;

out float occlusion;

layout (std140, binding = 1) uniform globals_ubo {
	mat4 projection;
	mat4 view;
	vec3 camera_pos;
	uvec3 grid_sz;				// cluster dimensions (xyz)
	uvec2 screen_dims;			// screen [ width, height ]
	float far_z;
	float near_z;
};

uniform sampler2D gbuf_pos;
uniform sampler2D gbuf_norms;
uniform sampler2D noise_tex;
uniform vec2 noise_scale;

// contains identifiers for each light
layout(std430, binding=10) buffer kernel {
	vec4 samples[];
};

void main() {

	vec3 pos = vec3(view * vec4(texture(gbuf_pos, fs_in.tex_coords).rgb, 1.0f));	// [ world -> view ]
	vec3 norm = mat3(view) * texture(gbuf_norms, fs_in.tex_coords).rgb;				// [ world -> view ]
	vec3 rand = texture(noise_tex, fs_in.tex_coords * noise_scale).rgb;

	vec3 tangent = rand - norm * dot(rand, norm);
	vec3 bitangent = cross(norm, tangent);
	mat3 tbn = mat3(tangent, bitangent, norm);

	occlusion = 0.0f;
	const float radius = 0.5f;
	const float bias = 0.025f;

	for (int i = 0; i < samples.length(); ++i) {
		vec3 curr_sample = tbn * samples[i].xyz;
		curr_sample = pos + curr_sample * radius;										  // [ tangent -> view ]
		vec4 offset = projection * vec4(curr_sample, 1.0f);								  // [ view -> clip ]
		offset.xyz /= offset.w; 
		offset.xyz = offset.xyz * 0.5f + 0.5f;											  // [ -1, 1 ] -> [ 0, 1 ]
		float sample_depth = vec3(view * vec4(texture(gbuf_pos, offset.xy).xyz, 1.0)).z;  // [ world -> view ]
		float range = smoothstep(0.0f, 1.0f, radius / abs(curr_sample.z - pos.z));		  // remove values outside of radius
		occlusion += ((sample_depth > curr_sample.z + bias) ? 1.0f : 0.0f) * range;
	}
	
	occlusion = 1.0 - (occlusion / samples.length());
}