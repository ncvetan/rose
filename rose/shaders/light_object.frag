#version 400 core

struct Material {
	sampler2D	diffuse_map;
	sampler2D	specular_map;
	float		shine_factor;
};

struct DirLight {
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct PointLight {
	vec3 position;
	
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float attn_const;
	float attn_lin;
	float attn_quad;
};

struct SpotLight {
	vec3 position;
	vec3 direction;
	
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float inner_cutoff;
	float outer_cutoff;

	float attn_const;
	float attn_lin;
	float attn_quad;
};

in vec3 frag_pos;
in vec3 normal;
in vec2 tex_coords;

out vec4 frag_color;

uniform Material material;
uniform DirLight dir_light;
#define NR_POINT_LIGHTS 4
uniform PointLight point_lights[NR_POINT_LIGHTS];
uniform SpotLight spot_light;
uniform vec3 view_pos;

vec3 calc_dir_light(DirLight light, vec3 normal, vec3 view_dir) {
	
	vec3    light_dir = normalize(-light.direction);
	
	vec3    ambient_rgb = light.ambient * texture(material.diffuse_map, tex_coords).rgb;
	
	float	diffuse_strength = max(dot(normal, light_dir), 0.0);
	vec3	diffuse_rgb = light.diffuse * diffuse_strength * texture(material.diffuse_map, tex_coords).rgb;

	vec3	reflect_dir = reflect(-light_dir, normal);
	float	specular_strength = pow(max(dot(view_dir, reflect_dir), 0.0), material.shine_factor);
	vec3	specular_rgb = light.specular * specular_strength * texture(material.specular_map, tex_coords).rgb;

	return  (ambient_rgb + diffuse_rgb + specular_rgb);
}

vec3 calc_point_light(PointLight light, vec3 normal, vec3 view_dir, vec3 frag_pos) {
	
	vec3    light_dir = normalize(light.position - frag_pos);
	
	vec3    ambient_rgb = light.ambient * texture(material.diffuse_map, tex_coords).rgb;
	
	float	diffuse_strength = max(dot(normal, light_dir), 0.0);
	vec3	diffuse_rgb = light.diffuse * diffuse_strength * texture(material.diffuse_map, tex_coords).rgb;

	vec3	reflect_dir = reflect(-light_dir, normal);
	float	specular_strength = pow(max(dot(view_dir, reflect_dir), 0.0), material.shine_factor);
	vec3	specular_rgb = specular_strength * texture(material.specular_map, tex_coords).rgb * light.specular;

	float   d = length(light.position - frag_pos);
	float   attenuation = 1.0 / (light.attn_const + light.attn_lin * d + light.attn_quad * pow(d, 2));

	return vec3((ambient_rgb + diffuse_rgb + specular_rgb) * attenuation);
}

vec3 calc_spot_light(SpotLight light, vec3 normal, vec3 view_dir, vec3 frag_pos) { 

	vec3 light_dir = normalize(light.position - frag_pos);
    vec3 reflect_dir = reflect(-light_dir, normal);

    float d = length(light.position - frag_pos);
    float attenuation = 1.0 / (light.attn_const + light.attn_lin * d + light.attn_quad * pow(d, 2));    

    float theta = dot(light_dir, normalize(-light.direction)); 
    float epsilon = light.inner_cutoff - light.outer_cutoff;
    float intensity = clamp((theta - light.outer_cutoff) / epsilon, 0.0, 1.0);
    
    vec3 ambient = light.ambient * texture(material.diffuse_map, tex_coords).rgb;

    float diffuse_strength = max(dot(normal, light_dir), 0.0);
    vec3 diffuse = light.diffuse * diffuse_strength * texture(material.diffuse_map, tex_coords).rgb;

    float specular_strength = pow(max(dot(view_dir, reflect_dir), 0.0), material.shine_factor);
    vec3 specular = light.specular * specular_strength * texture(material.specular_map, tex_coords).rgb;

    return (ambient + diffuse + specular) * intensity * attenuation;
}

void main() {

	vec3 norm = normalize(normal);
	vec3 view_dir = normalize(view_pos - frag_pos);

	vec3 result = calc_dir_light(dir_light, norm, view_dir);

	for (int i = 0; i < NR_POINT_LIGHTS; ++i){
		result += calc_point_light(point_lights[i], norm, view_dir, normalize(frag_pos));
	}

	result += calc_spot_light(spot_light, norm, view_dir, frag_pos);
	frag_color = vec4(result, 1.0);
}