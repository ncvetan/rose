// =============================================================================
//   ImGui gui code for Rose
// =============================================================================

#ifndef ROSE_INCLUDE_GUI
#define ROSE_INCLUDE_GUI

#include <rose/app_state.hpp>
#include <rose/camera.hpp>

namespace rose {

struct GL_Platform;

namespace gui {

void gl_imgui(AppState& app_state, GL_Platform& platform);

}
} // namespace rose

#endif