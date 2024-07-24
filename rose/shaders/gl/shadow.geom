#version 460 core

layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

uniform mat4 shadow_mats[6];

out vec4 frag_pos;

void main() {
	for (int i = 0; i < 6; ++i) {		// faces
		gl_Layer = i;
		for (int j = 0; j < 3; ++j) {	// vertices
			frag_pos = gl_in[j].gl_Position;
			gl_Position = shadow_mats[i] * frag_pos;
			EmitVertex();
		}
		EndPrimitive();
	}
}