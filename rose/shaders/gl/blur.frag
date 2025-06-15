#version 460 core

out float occlusion;
  
in vs_data {
	vec2 tex_coords;
} fs_in;
  
uniform sampler2D occlusion_tex;

void main() {
    
    vec2 texel_sz = 1.0f / vec2(textureSize(occlusion_tex, 0));
    float ret = 0.0f;

    for (int x = -2; x < 2; ++x) 
    {
        for (int y = -2; y < 2; ++y) 
        {
            ret += texture(occlusion_tex, fs_in.tex_coords + vec2(float(x), float(y)) * texel_sz).r;
        }
    }
    occlusion = ret / 16.0f;
}  