#ifndef ROSE_INCLUDE_APPLICATION
#define ROSE_INCLUDE_APPLICATION

#include <rose/app_state.hpp>
#include <rose/core/err.hpp>
#include <rose/gl/gl_platform.hpp>

#include <backends/imgui_impl_glfw.h>

#include <concepts>
#include <optional>

namespace rose {

std::optional<rses> init_glfw(WindowData& window_data);
std::optional<rses> init_imgui(WindowData& window_data);

struct RoseApp {
    AppData app_data;

    template <typename T>
    std::optional<rses> init(T& platform) {
        std::optional<rses> err = std::nullopt;

        if (err = init_glfw(app_data.window_data)) {
            return err.value().general("Unable to initialize GLFW");
        }

        if (err = init_imgui(app_data.window_data)) {
            return err.value().general("Unable to initialize Dear ImGui");
        }

        if (err = platform.init(app_data)) {
            return err.value().general("Unable to initialize application");
        }
        return std::nullopt;
    }
    
    template <typename T>
    void run(T& platform) {
        while (!glfwWindowShouldClose(app_data.window_data.window_handle)) {
            handle_events();
            platform.update(app_data);
            glfwSwapBuffers(app_data.window_data.window_handle);
            glfwPollEvents();
        }
    }

    template <typename T>
    void finish(T& platform) {
        platform.finish();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(app_data.window_data.window_handle);
        glfwTerminate();    
    }
    
    void handle_events();

};
} // namespace rose

#endif