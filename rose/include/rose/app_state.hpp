#ifndef ROSE_INCLUDE_APP_STATE
#define ROSE_INCLUDE_APP_STATE

#include <rose/camera.hpp>
#include <rose/core/math.hpp>

#include <GLFW/glfw3.h>
#include <imgui.h>

namespace rose {

struct WindowData {
    GLFWwindow* window_handle = nullptr;

    u32 width = 1920;
    u32 height = 1080;
    ImGuiID dock_id = 0;
    std::string name = "Rose";

    vec2f last_xy;
    vec2f mouse_xy;

    Rectf vp_rect;
    bool vp_focused = false;
    bool vp_captured = true; // indicates whether events should be processed in the viewport

    f32 gamma = 2.2f;
    f32 exposure = 1.0f;
};

struct AppData {
    WindowData window_data;
    Camera camera;

    bool bloom_on = true;
    i32 n_bloom_passes = 5;
};

}
#endif