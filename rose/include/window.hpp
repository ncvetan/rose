#ifndef ROSE_INCLUDE_WINDOW
#define ROSE_INCLUDE_WINDOW

#include <optional>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <camera.hpp>
#include <shader.hpp>
#include <err.hpp>

namespace rose {

class WindowGLFW {
  public:
    WindowGLFW() = default;

    std::optional<rses> init();
    void enable_vsync(bool enable);
    void update();
    void destroy();

    GLFWwindow* window = nullptr;
    ShaderGL light_object_shader{};
    ShaderGL light_source_shader{};
    CameraGL camera{};
    unsigned int diffuse_map;
    unsigned int specular_map;
    unsigned int emission_map;

    uint16_t width = 1280;
    uint16_t height = 720;
    std::string name = "Rose";
    glm::vec2 last_xy = { 0.0f, 0.0f };
};

void resize_callback(GLFWwindow* window, int width, int height);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void mouse_callback(GLFWwindow* window, double xpos, double ypos);

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

void process_input(GLFWwindow* window, float delta_time);
} // namespace rose

#endif
