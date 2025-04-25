// =============================================================================
//   contains structures used in Rose's lighting systems
// =============================================================================

#ifndef ROSE_INCLUDE_LIGHTING
#define ROSE_INCLUDE_LIGHTING

#include <rose/core/core.hpp>
#include <rose/gl/structs.hpp>

#include <glm.hpp>

// TODO: decouple OpenGL buffers from non platform specific state of lighting system

struct DirShadowData {
    u32 fbo = 0;
    u32 tex = 0;
    u32 light_mats_ubo = 0; // transforms matrices to directional light space
    u16 resolution = 2048;
    u8 n_cascades = 3;
};

std::optional<rses> init_dir_shadow(DirShadowData& shadow);

struct DirLight {
    glm::vec3 direction = { 0.0f, -0.999848f, -0.0174525f };
    glm::vec3 color = { 0.55f, 0.55f, 0.55f };
    DirShadowData shadow_data;
};

// obtains a projection-view matrix for a directional light
glm::mat4 get_dir_light_mat(const glm::mat4& proj, const glm::mat4& view, const DirLight& light);

struct PtShadowData {
    u32 fbo = 0;
    u32 tex = 0;
    u16 resolution = 2048;
};

std::optional<rses> init_pt_shadow(PtShadowData& shadow);

// state for a singular point light
struct PtLight {

    // calculate the radius of the point light
    inline void radius() {
        f32 lum = glm::dot(glm::vec3(color.r, color.g, color.b), glm::vec3(0.2126, 0.7152, 0.0722));
        f32 threshold = 0.01f;
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
    gl::SSBO clusters_aabb_ssbo;             // AABBs for each cluster
    gl::SSBO lights_ssbo;                    // light parameters for each light in the scene
    gl::SSBO lights_pos_ssbo;                // light positions for each light in the scene
    gl::SSBO clusters_ssbo;                  // light for each cluster
    glm::uvec3 grid_sz = { 16, 9, 24 };  // size of cluster grid (xyz)
    u32 max_lights_in_cluster = 100;     // number of lights that will be considered for a single cluster
};

#endif