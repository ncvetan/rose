#ifndef ROSE_INCLUDE_WINDOW
#define ROSE_INCLUDE_WINDOW

#include <rose/camera.hpp>
#include <rose/entities.hpp>
#include <rose/model.hpp>
#include <rose/core/err.hpp>
#include <rose/core/math.hpp>
#include <rose/gl/shader.hpp>
#include <rose/gl/structs.hpp>

#include <GLFW/glfw3.h>
#include <imgui.h>

#include <optional>
#include <unordered_map>

namespace rose {

struct WindowData {
    u32 width = 1920;
    u32 height = 1080;
    ImGuiID dock_id = 0;
    std::string name = "Rose";

    vec2f last_xy;
    vec2f mouse_xy;

    Rectf vp_rect;
    bool vp_focused = false;
    bool vp_captured = true; // indicates whether events should be processed in the viewport
};

struct WindowGLFW {
    
    WindowGLFW() = default;

    void enable_vsync(bool enable);

    [[nodiscard]] std::optional<rses> init();

    GLFWwindow* window = nullptr;
    WindowData window_data;
    
    void run();
    void finish();
    void handle_events();

    FrameBuf gbuf;
    FrameBuf pp1;
    FrameBuf fbuf_out;
    ClusterData clusters;

    Camera camera;
    ShadersGL shaders;
    TextureManager texture_manager;

    GlobalState app_state;
    Entities entities;

};

} // namespace rose

#endif
