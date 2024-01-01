#version 400 core

struct Material {
	sampler2D	diffuse;
	sampler2D	specular;
	float		shine_factor;
};

struct Light {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	vec3  direction;
	float cutoff;

	float attn_const;
	float attn_lin;
	float attn_quad;
};

in vec3 frag_pos;
in vec3 normal;
in vec2 tex_coords;

out vec4 frag_color;

uniform Material material;
uniform Light light;

void main() {

	vec3    light_dir = normalize(-frag_pos);
	float   theta = dot(light_dir, vec3(0, 0, 1));

	vec3 ambient = light.ambient * texture(material.diffuse, tex_coords).rgb;
	vec4 result;

	if (theta > light.cutoff) {
		
		// diffuse
		vec3	norm = normalize(normal);
		float	diffuse_strength = max(dot(norm, light_dir), 0.0);
		vec3	diffuse = diffuse_strength * texture(material.diffuse, tex_coords).rgb * light.diffuse;
	
		// specular
		vec3	reflect_dir = reflect(-light_dir, norm);
		vec3	view_dir = normalize(-frag_pos);
		float	specular_strength = pow(max(dot(view_dir, reflect_dir), 0.0), material.shine_factor);
		vec3	specular = specular_strength * texture(material.specular, tex_coords).rgb * light.specular;

		// attenuation
		float   d = length(frag_pos);
		float   attenuation = 1 / (light.attn_const + light.attn_lin * d + light.attn_quad * pow(d, 2));

		result = vec4(ambient + (diffuse + specular) * attenuation, 1.0);
	}
	else {
		result = vec4(ambient, 1.0);
	}

	frag_color = result;
};