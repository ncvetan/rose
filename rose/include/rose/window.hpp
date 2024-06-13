#ifndef ROSE_INCLUDE_WINDOW
#define ROSE_INCLUDE_WINDOW

#include <rose/camera.hpp>
#include <rose/err.hpp>
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

class WindowGLFW {
  public:
    WindowGLFW() = default;

    std::optional<rses> init();
    void enable_vsync(bool enable);
    void update();
    void destroy();

    GLFWwindow* window = nullptr;
    CameraGL camera;
    std::unordered_map<std::string, ShaderGL> shaders;

    GlobalState world_state;
    std::vector<Object<Model>> objects;
    std::vector<Object<TexturedCube>> tex_cubes;
    std::vector<Object<Cube>> pnt_lights;

    u32 fbo = 0;
    u32 rbo = 0;
    TextureRef fbo_tex;
    u16 width = 1920;
    u16 height = 1080;
    ImGuiID dock_id = 0;
    std::string name = "Rose";
    glm::vec2 last_xy;
    float gamma = 2.2;

    vec2f mouse_xy;
    Rectf vp_rect;
    bool vp_focused = false;
    bool glfw_captured = true;
};

void resize_callback(GLFWwindow* window, int width, int height);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void mouse_callback(GLFWwindow* window, double xpos, double ypos);

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

void process_input(GLFWwindow* window, float delta_time);

} // namespace rose

#endif
