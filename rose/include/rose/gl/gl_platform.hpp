// =============================================================================
//   code specific to the OpenGL platform
// =============================================================================

#ifndef ROSE_INCLUDE_GL_PLATFORM
#define ROSE_INCLUDE_GL_PLATFORM

#include <rose/app_state.hpp>
#include <rose/camera.hpp>
#include <rose/entities.hpp>
#include <rose/model.hpp>
#include <rose/core/err.hpp>
#include <rose/core/math.hpp>
#include <rose/gl/shader.hpp>
#include <rose/gl/structs.hpp>

#include <GLFW/glfw3.h>
#include <imgui.h>

// TODO: There is still a ton of unnecessary coupling going on here

namespace rose {

// state specific to the OpenGL platform
struct GL_PlatformState {
    SkyBox sky_box;
    u32 global_ubo = 0; // ubo storing values available across shaders
    DirLight dir_light;
    ShadowData pt_shadow;
};

struct GL_Platform {
    
    [[nodiscard]] std::optional<rses> init(AppState& app_state);
    void update(AppState& app_state);
    void finish();
    void enable_vsync(bool enable);

    // note: destruction order is important
    // entities must be destructed before texture managers
    TextureManager texture_manager;
    GL_PlatformState platform_state;
    Entities entities;
    GL_Shaders shaders;

    FrameBuf gbuf_fbuf;  // gbuffers
    FrameBuf int_fbuf;   // intermediate
    FrameBuf out_fbuf;   // output
    ClusterData clusters;
};

} // namespace rose

#endif
