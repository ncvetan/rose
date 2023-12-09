#version 400 core

struct Material {
	sampler2D diffuse;
	vec3 specular;
	float shine_factor;
};

struct Light {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

in vec3 frag_pos;
in vec3 normal;
in vec3 light_pos;
in vec2 tex_coords;

out vec4 frag_color;

uniform Material material;
uniform Light light;

void main() {

	// diffuse
	vec3 norm = normalize(normal);
	vec3 light_dir = normalize(light_pos - frag_pos);
	float diffuse_strength = max(dot(norm, light_dir), 0.0);
	vec3 diffuse = diffuse_strength * texture(material.diffuse, tex_coords).rgb * light.diffuse;
	
	// ambient
	vec3 ambient = light.ambient * texture(material.diffuse, tex_coords).rgb;

	// specular
	vec3 view_dir = normalize(-frag_pos);
	vec3 reflect_dir = reflect(-light_dir, normal);
	float specular_strength = pow(max(dot(view_dir, reflect_dir), 0.0), material.shine_factor);
	vec3 specular = specular_strength * material.specular * light.specular;

	vec3 result = ambient + diffuse + specular;
	frag_color = vec4(result, 1.0);
};