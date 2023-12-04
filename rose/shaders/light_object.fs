#version 400 core

in vec3 frag_pos;
in vec3 normal;
in vec3 light_pos_view;

out vec4 frag_color;

uniform vec3 light_color;
uniform vec3 object_color;

void main() {
	// ambient
	float ambient_strength = 0.1;
	vec3 ambient = ambient_strength * light_color;

	// diffuse
	vec3 norm = normalize(normal);
	vec3 light_dir = normalize(light_pos_view - frag_pos);
	float diffuse_strength = max(dot(norm, light_dir), 0.0);
	vec3 diffuse = diffuse_strength * light_color;

	// specular
	vec3 view_dir = normalize(-frag_pos);
	vec3 reflect_dir = reflect(-light_dir, normal);
	float specular_strength = 0.5 * pow(max(dot(view_dir, reflect_dir), 0.0), 32);
	vec3 specular = specular_strength * light_color;

	vec3 result = (ambient + diffuse + specular) * object_color;
	frag_color = vec4(result, 1.0);
};