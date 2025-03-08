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

namespace rose {

// state specific to the OpenGL platform
struct GL_PlatformState {
    SkyBox sky_box;
    u32 global_ubo = 0; // ubo storing values available across shaders
    DirLight dir_light;
    ShadowData pt_shadow;
};

struct GL_Platform {
    
    [[nodiscard]] std::optional<rses> init(AppData& app_data);
    void update(AppData& app_data);
    void finish();
    void enable_vsync(bool enable);

    TextureManager texture_manager;
    GL_PlatformState platform_state;
    Entities entities;
    GL_Shaders shaders;

    FrameBuf gbuf;
    FrameBuf pp1;
    FrameBuf fbuf_out;
    ClusterData clusters;
};

} // namespace rose

#endif
