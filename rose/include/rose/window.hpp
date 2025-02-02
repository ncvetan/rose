#ifndef ROSE_INCLUDE_WINDOW
#define ROSE_INCLUDE_WINDOW

#include <rose/camera.hpp>
#include <rose/err.hpp>
#include <rose/glstructs.hpp>
#include <rose/math.hpp>
#include <rose/model.hpp>
#include <rose/object.hpp>
#include <rose/shader.hpp>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <optional>
#include <unordered_map>

namespace rose {

// TODO: This entire thing is much more than a 'window' at this point and needs to be refactored
class WindowGLFW {
  public:
    WindowGLFW() = default;

    [[nodiscard]] std::optional<rses> init();
    void enable_vsync(bool enable);
    void update();
    void shutdown();
    void handle_events();

    GLFWwindow* window = nullptr;
    CameraGL camera;
    ShadersGL shaders;
    TextureManager texture_manager;

    GlobalState world_state;
    ClusterData clusters;
    Objects objects;

    FrameBuf gbuf;
    FrameBuf pp1;
    FrameBuf pp2;
    FrameBuf fbuf_out;

    u32 width = 1920;
    u32 height = 1080;
    ImGuiID dock_id = 0;
    std::string name = "Rose";
    
    vec2f last_xy;
    vec2f mouse_xy;

    Rectf vp_rect;
    bool vp_focused = false;
    bool vp_captured = true;  // indicates whether events should be processed in the viewport

  private:
    [[nodiscard]] std::optional<rses> init_glfw();
    [[nodiscard]] std::optional<rses> init_imgui(GLFWwindow* window);
    [[nodiscard]] std::optional<rses> init_opengl();
};

void scroll_callback(GLFWwindow* window, f64 xoffset, f64 yoffset);


} // namespace rose

#endif
