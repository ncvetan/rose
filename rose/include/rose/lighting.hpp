// =============================================================================
//   contains structures used in Rose's lighting systems
// =============================================================================

#ifndef ROSE_INCLUDE_LIGHTING
#define ROSE_INCLUDE_LIGHTING

#include <rose/alias.hpp>
#include <rose/glstructs.hpp>

#include <glm.hpp>

namespace rose {

struct DirShadowData {
    u32 fbo = 0;
    u32 tex = 0;
    u16 resolution = 2048;
    u8 n_cascades = 3;
};

struct ShadowData {
    u32 fbo = 0;
    u32 tex = 0;
    u16 resolution = 2048;
};

struct DirLight {
    glm::vec3 direction = { 0.0f, -0.999848f, -0.0174525f };
    glm::vec3 color = { 0.55f, 0.55f, 0.55f };
    DirShadowData shadow;
    u32 light_mats_ubo = 0;     // transforms matrices to directional light space
};

// state for a singular point light
struct PtLight {

    // calculate the radius of the point light
    inline void radius(f32 exposure) {
        f32 lum = glm::dot(glm::vec3(color.r, color.g, color.b), glm::vec3(0.2126, 0.7152, 0.0722));
        f32 threshold = 0.01f / exposure;
        rad = (-linear + std::sqrtf(linear * linear - 4.0f * quad * (1.0f - lum / threshold)) / (2.0f * quad));
    };

    glm::vec4 color = { 0.20f, 0.20f, 0.20f, 1.0f };
    f32 linear = 1.0f;
    f32 quad = 0.7f;
    f32 intensity = 1.8f;
    f32 rad = 1.0f;
};

// state used for clustered shading
struct ClusterData {
    SSBO clusters_aabb_ssbo;             // AABBs for each cluster
    SSBO lights_ssbo;                    // light parameters for each light in the scene
    SSBO lights_pos_ssbo;                // light positions for each light in the scene
    SSBO clusters_ssbo;                  // light for each cluster
    glm::uvec3 grid_sz = { 16, 9, 24 };  // size of cluster grid (xyz)
    u32 max_lights_in_cluster = 100;     // number of lights that will be considered for a single cluster
};

}

#endif