#ifndef ROSE_INCLUDE_LIGHTING
#define ROSE_INCLUDE_LIGHTING

#include <rose/alias.hpp>

#include <glm.hpp>

namespace rose {

 // TODO: Put this somewhere else
struct AABB {
    glm::vec4 max_pt;
    glm::vec4 min_pt;
};

// data related to cluster shading
struct ClusterCtx {
    glm::uvec3 grid_sz = { 16, 9, 24 }; // size of cluster grid (xyz)
    s32 max_lights_in_cluster = 100;    // number of lights that will be considered for a single cluster
    u32 clusters_aabb_ssbo;             // AABBs for each cluster
    u32 clusters_indices_ssbo;          // indices into lights for each cluster
    u32 light_grid_ssbo;                // offset into indices and number to read for each cluster
    u32 lights_ssbo;                    // light parameters for each light in the scene
    u32 lights_pos_ssbo;                // light positions for each light in the scene
    u32 light_idx_ssbo;                 // state for inter-workgroup communication for writing into clusters_indices_ssbo

    u32 clusters_color_ssbo;            // TODO: delete - temporary, state for inter-workgroup communication for writing into clusters_indices_ssbo

    std::vector<AABB> aabbs;
};

struct DirLight {
    glm::vec3 direction = { 0.0f, -0.999848f, -0.0174525f };
    glm::vec3 color = { 0.025f, 0.025f, 0.025f };
};

struct PointLight {

    // calculate the radius of the point light
    inline void radius() {
        rad = (-linear + std::sqrtf(linear * linear - 4 * quad * (1.0f - (256.0f / 5.0f) * std::max({
            color.r, color.g, color.b}))) / (2.0f * quad));
    };

    glm::vec4 color = { 0.20f, 0.20f, 0.20f, 1.0f };
    float linear = 1.0f;
    float quad = 0.7f;
    float intensity = 1.8f;
    float rad = 1.0f;
};

struct ShadowCtx {
    u32 fbo = 0;
    u32 tex = 0;
    float bias = 0.005;
    u16 resolution = 2048;
};

}

#endif