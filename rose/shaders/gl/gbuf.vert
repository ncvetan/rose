#version 460 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec2 tex_coords;

out vs_data {
	vec3 frag_pos_ws;		// world space
	vec3 frag_pos_ts;		// tangent space
	vec3 normal;			// tangent space
	vec3 view_pos_ts;		// tangent space
	vec2 tex_coords;
} vs_out;

struct DirLight {
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

layout (std140, binding = 1) uniform globals {
	mat4 projection;
	mat4 view;
	vec3 camera_pos;
	DirLight dir_light;
};

uniform mat4 model;

void main() {
	
	mat3 normal_mat = mat3(transpose(inverse(mat3(model))));

	vec3 t = normalize(normal_mat * tangent);
	vec3 n = normalize(normal_mat * normal);
	t = normalize(t - dot(t, n) * n);		// re-orthogonalize
	vec3 b = cross(n, t);
	mat3 tbn = transpose(mat3(t, b, n));	// ortho transpose = inverse
	
	vs_out.frag_pos_ws = vec3(model * vec4(pos, 1.0));
	vs_out.frag_pos_ts = tbn * vs_out.frag_pos_ws;
	vs_out.normal = normal_mat * normal;
	vs_out.view_pos_ts = tbn * camera_pos;
	vs_out.tex_coords = tex_coords;

	gl_Position = projection * view * vec4(model * vec4(pos, 1.0));
};