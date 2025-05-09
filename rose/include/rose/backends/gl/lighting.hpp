// =============================================================================
//   lighting code specific to the OpenGL backend
// =============================================================================

#ifndef ROSE_INCLUDE_BACKENDS_GL_LIGHTING
#define ROSE_INCLUDE_BACKENDS_GL_LIGHTING

#include <rose/core/core.hpp>
#include <rose/backends/gl/structs.hpp>

namespace gl {

struct DirShadowData {

    rses init();

    u32 fbo = 0;
    u32 tex = 0;
    u32 light_mats_ubo = 0; // transform matrices to directional light space
    u16 resolution = 2048;
    u8 n_cascades = 3;
};

struct PtShadowData {

    rses init();

    u32 fbo = 0;
    u32 tex = 0;
    u16 resolution = 2048;
};

struct ClustersData {
    gl::SSBO aabb_ssbo;          // AABBs for each cluster
    gl::SSBO lights_ssbo;        // light parameters for each light in the scene
    gl::SSBO lights_pos_ssbo;    // light positions for each light in the scene
    gl::SSBO clusters_ssbo;      // light for each cluster
};

} // namespace gl

#endif