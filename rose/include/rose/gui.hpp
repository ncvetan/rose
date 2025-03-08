#ifndef ROSE_INCLUDE_GUI
#define ROSE_INCLUDE_GUI

#include <rose/app_state.hpp>
#include <rose/camera.hpp>

namespace rose {

struct GLPlatform;

namespace gui {

void gl_imgui(AppData& app_data, GLPlatform& platform);

}
} // namespace rose

#endif