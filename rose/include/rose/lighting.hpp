#ifndef ROSE_INCLUDE_LIGHTING
#define ROSE_INCLUDE_LIGHTING

#include <rose/alias.hpp>

#include <glm.hpp>

namespace rose {

// data related to cluster shading
struct ClusterCtx {
    glm::vec3 grid_sz = { 16, 9, 4 };
    i32 n_clusters = 16 * 9 * 24;
    i32 max_lights_in_cluster = 100;
    u32 clusters_aabb_ssbo;    // AABBs for each cluster
    u32 clusters_indices_ssbo; // indices into lights for each cluster
    u32 light_grid_ssbo;       // offset into indices and number to read for each cluster
    u32 lights_ssbo;           // light parameters for each light in the scene
    u32 lights_pos_ssbo;           // light positions for each light in the scene
    u32 light_idx_ssbo;        // state for inter-workgroup communication for writing into clusters_indices_ssbo
    std::vector<AABB> aabbs;
};

struct DirLight {
    glm::vec3 direction = { 0.0f, -0.999848f, -0.0174525f };
    glm::vec3 color = { 0.025f, 0.025f, 0.025f };
};

struct PointLight {
    inline float radius() {
        float f = (-linear + std::sqrtf(linear * linear - 4 * quad * (1.0f - (256.0f / 5.0f) * std::max({
            color.r, color.g, color.b}))) / (2.0f * quad));
        return f;
    };

    alignas(16) glm::vec3 color = { 0.20f, 0.20f, 0.20f };
    float linear = 1.0f;
    float quad = 0.7f;
    float intensity = 1.8f;
};

struct ShadowCtx {
    u32 fbo = 0;
    u32 tex = 0;
    float bias = 0.005;
    u16 resolution = 2048;
};

}

#endif