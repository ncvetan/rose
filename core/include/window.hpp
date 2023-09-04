#pragma once

#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <camera.hpp>
#include <shader.hpp>

namespace rose
{

    static ShaderGL shader{};
    static CameraGL camera{};
    /*
    For now, only GLFW windows are supported
    */

    // Interface
    class Window
    {
      public:
        Window() = default;

        virtual ~Window() = default;

        virtual bool init() = 0;

        virtual void enable_vsync(bool enable) = 0;

        virtual void update() = 0;

        virtual void destroy() = 0;

        // virtual void handle_event(int event) = 0;
    };

    class WindowGLFW : public Window
    {
      public:
        WindowGLFW() = default;
        WindowGLFW(std::string name, int height, int width);

        virtual ~WindowGLFW(){};

        virtual bool init() override;

        virtual void enable_vsync(bool enable) override;

        virtual void update() override;

        virtual void destroy() override;

        // virtual void handle_event(int event) override;

        double get_time();

        GLFWwindow* window = nullptr;
        // CameraGL camera{};
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