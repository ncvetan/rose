// =============================================================================
//   shader for rendering to cascaded shadow maps for directional lighting
// =============================================================================

// note: this shader enables layered rendering to textures for each cascade

#version 460 core

layout(triangles, invocations = 3) in;
layout(triangle_strip, max_vertices = 3) out;

layout(std140, binding=6) uniform light_space_mats_ubo {
	mat4 ls_mats[3];
};

void main() {
	for (int i=0; i < 3; ++i) {
		gl_Position = ls_mats[gl_InvocationID] * gl_in[i].gl_Position;
		gl_Layer = gl_InvocationID;
		EmitVertex();
	}
	EndPrimitive();
}