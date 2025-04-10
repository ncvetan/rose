// =============================================================================
//   simple shader to render point lights
// =============================================================================

#version 460 core

layout (location = 0) out vec4 frag_color;


uniform vec4 color;
uniform float intensity;

void main() {
	frag_color = vec4(color.rgb * intensity, 1.0);
}