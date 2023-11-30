#version 330 core

in vec3 frag_pos;
in vec3 normal;

out vec4 frag_color;

uniform vec3 light_pos;
uniform vec3 light_color;
uniform vec3 object_color;

void main()
{
	float ambient_strength = 0.1;
	vec3 ambient = ambient_strength * light_color;

	vec3 norm = normalize(normal);
	vec3 light_dir = normalize(light_pos - frag_pos);
	float diffuse_strength = max(dot(norm, light_dir), 0.0);
	vec3 diffuse = diffuse_strength * light_color;

	vec3 result = (ambient + diffuse) * object_color;
	frag_color = vec4(result, 1.0);
};