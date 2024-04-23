#ifndef ROSE_INCLUDE_GUI
#define ROSE_INCLUDE_GUI

#include <rose/camera.hpp>

namespace rose {

struct WindowGLFW;

namespace gui {

void imgui(WindowGLFW& state);

}
} // namespace rose

#endif