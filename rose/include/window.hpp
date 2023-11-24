#ifndef ROSE_INCLUDE_WINDOW
#define ROSE_INCLUDE_WINDOW

#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <camera.hpp>
#include <shader.hpp>

namespace rose {

// Interface
template <typename T>
class Window {
  public:
    Window() = default;

    inline bool init() { return static_cast<T*>(this)->init(); };

    inline void enable_vsync(bool enable) { static_cast<T*>(this)->enable_vsync(); };

    inline void update() { static_cast<T*>(this)->update(); };

    inline void destroy() { static_cast<T*>(this)->destroy(); };
};

class WindowGLFW : public Window<WindowGLFW> {
  public:
    WindowGLFW() = default;

    bool init();
    void enable_vsync(bool enable);
    void update();
    void destroy();

    double get_time();

    GLFWwindow* window = nullptr;
    ShaderGL shader{};
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
