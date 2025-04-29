// =============================================================================
//   ImGui gui code for Rose
// =============================================================================

#ifndef ROSE_INCLUDE_GUI
#define ROSE_INCLUDE_GUI

#include <rose/app_state.hpp>
#include <rose/camera.hpp>
#include <rose/gl/platform.hpp>

namespace gui {

struct GuiRet {
    bool light_changed = false; // indicated whether a light was changed from within the GUI
};

GuiRet imgui(AppState& app_state, gl::Platform& platform);

}

#endif