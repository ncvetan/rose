// =============================================================================
//   contains structures used in Rose's lighting systems
// =============================================================================

#ifndef ROSE_INCLUDE_LIGHTING
#define ROSE_INCLUDE_LIGHTING

#include <rose/core/core.hpp>

#ifdef USE_OPENGL
#include <rose/backends/gl/structs.hpp>
#include <rose/backends/gl/lighting.hpp>
#else
static_assert("no backend selected");
#endif

#include <glm.hpp>

struct DirLight {
    glm::vec3 direction = { 0.0f, -0.999848f, -0.0174525f };
    glm::vec3 color = { 0.55f, 0.55f, 0.55f };
    
#ifdef USE_OPENGL
    gl::DirShadowData gl_shadow;
#else
    static_assert("no backend selected");
#endif
};

// obtains a projection-view matrix for a directional light
glm::mat4 get_cascade_mat(const glm::mat4& proj, const glm::mat4& view, glm::vec3 direction, f32 resolution);

// state for a singular point light
// 
// note: padding added to meet std430 layout requirements
struct PtLight {
    glm::vec4 color = { 0.5f, 0.5f, 0.5f, 1.0f };
    f32 radius = 25.0f;
    f32 intensity = 1.0f;
    u8 padding[8] = { 0 };
};

// state used for clustered shading
struct Clusters {
    glm::uvec3 grid_sz = { 16, 9, 24 };      // size of cluster grid (xyz)
    u32 max_lights_in_cluster = 100;         // number of lights that will be considered for a single cluster

#ifdef USE_OPENGL
    gl::ClustersData gl_data;
#else
    static_assert("no backend selected");
#endif
};

#endif