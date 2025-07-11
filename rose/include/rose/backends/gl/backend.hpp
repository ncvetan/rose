// =============================================================================
//   code specific to the OpenGL backend
// =============================================================================

#ifndef ROSE_INCLUDE_BACKENDS_GL_BACKEND
#define ROSE_INCLUDE_BACKENDS_GL_BACKEND

#include <rose/app_state.hpp>
#include <rose/camera.hpp>
#include <rose/entities.hpp>
#include <rose/model.hpp>
#include <rose/backends/gl/shader.hpp>
#include <rose/backends/gl/structs.hpp>
#include <rose/core/err.hpp>
#include <rose/core/types.hpp>

#include <GLFW/glfw3.h>
#include <imgui.h>

namespace gl {

// state specific to the OpenGL backend
struct BackendState {
    SkyBox skybox;
    u32 global_ubo = 0; // ubo storing values available across shaders
    DirLight dir_light;
    PtShadowData pt_shadow_data;
    std::vector<Mip> bloom_mip_chain;
    u32 ssao_noise_tex = 0;
    SSBO ssao_samples_ssbo;
};


struct Backend {

    rses init(AppState& app_state);
    void new_frame(AppState& app_state);
    void end_frame(GLFWwindow* window_handle);
    void step(AppState& app_state);
    void finish();

    // note: destruction order is important
    // entities must be destructed before texture managers
    TextureManager texture_manager;
    BackendState backend_state;
    Shaders shaders;
    Clusters clusters;

    SSBO lights_ids_ssbo;   // IDs for each point light

    FrameBuf gbuf_fbuf;     // gbuffers
    FrameBuf int_fbuf;      // intermediate
    FrameBuf ssao_fbuf;     // occlusion factor
    FrameBuf out_fbuf;      // output
};

} // namespace gl

#endif
