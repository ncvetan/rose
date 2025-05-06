// =============================================================================
//   ImGui gui code for Rose
// =============================================================================

#ifndef ROSE_INCLUDE_GUI
#define ROSE_INCLUDE_GUI

#include <rose/app_state.hpp>
#include <rose/camera.hpp>

#ifdef USE_OPENGL
#include <rose/backends/gl/backend.hpp>
#else
static_assert("no backend selected");
#endif 

namespace gui {

struct GuiRet {
    bool light_changed = false; // indicated whether a light was changed from within the GUI
};

GuiRet imgui(AppState& app_state, gl::Backend& backend);

}

#endif