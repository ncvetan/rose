// =============================================================================
//   simple shader to render point lights
// =============================================================================

#version 460 core

layout (location = 0) out vec4 frag_color;

void main() {
	// TODO: Adjust based on light color
	frag_color = vec4(1.0, 1.0, 1.0, 1.0);
}