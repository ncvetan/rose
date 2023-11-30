#ifndef ROSE_INCLUDE_WINDOW
#define ROSE_INCLUDE_WINDOW

#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <camera.hpp>
#include <shader.hpp>

namespace rose {

class WindowGLFW {
  public:
    WindowGLFW() = default;

    bool init();
    void enable_vsync(bool enable);
    void update();
    void destroy();

    double get_time();

    GLFWwindow* window = nullptr;
    ShaderGL texture_shader{};
    ShaderGL light_object_shader{};
    ShaderGL light_source_shader{};
    CameraGL camera{};

  private:
    unsigned int width = 1280;
    unsigned int height = 720;
    std::string name = "Rose";
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void mouse_callback(GLFWwindow* window, double xpos, double ypos);

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

void process_input(GLFWwindow* window, float delta_time);
} // namespace rose

#endif
