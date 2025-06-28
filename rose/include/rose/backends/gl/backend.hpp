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
    SkyBox skybox;                      // skybox model state
    UBO global_ubo;                     // ubo storing values available across shaders
    DirLight dir_light;                 // directional light parameters
    PtShadowData pt_shadow_data;        // render data associated with a point light shadow
    std::vector<Mip> bloom_mip_chain;   // chain of mip data
    TextureRef ssao_noise_tex;          // texture of noise used in SSAO
    SSBO ssao_samples_ssbo;             // SSBO containing samples for SSAO
};

struct Backend {

    rses init(AppState& app_state);
    void new_frame(AppState& app_state);
    void end_frame(GLFWwindow* window_handle);
    void step(AppState& app_state);
    void finish();

    // note: texture manager must be destructed last
    TextureManager texture_manager;
    BackendState backend_state;
    Shaders shaders;
    Clusters clusters;

    SSBO lights_ids_ssbo;   // IDs for each point light

    FBuf gbuf_fbuf;     // gbuffers
    FBuf int_fbuf;      // intermediate
    FBuf ssao_fbuf;     // occlusion factor
    FBuf out_fbuf;      // output
};

} // namespace gl

#endif
