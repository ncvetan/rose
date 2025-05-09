// =============================================================================
//   highest level code in Rose, used to handle the running of the application
// =============================================================================

#ifndef ROSE_INCLUDE_APP
#define ROSE_INCLUDE_APP

#include <rose/app_state.hpp>
#include <rose/core/err.hpp>

#include <backends/imgui_impl_glfw.h>

#include <concepts>
#include <optional>

rses init_glfw(WindowState& window_state);
rses init_imgui(WindowState& window_state);

struct RoseApp {
    AppState app_data;

    template <typename T>
    rses init(T& backend) {
        rses err;
        if (err = init_glfw(app_data.window_state)) {
            return err.general("unable to initialize GLFW");
        }
        if (err = init_imgui(app_data.window_state)) {
            return err.general("unable to initialize Dear ImGui");
        }
        if (err = backend.init(app_data)) {
            return err.general("unable to initialize application");
        }
        return {};
    }
    
    template <typename T>
    void run(T& backend) {
        while (!glfwWindowShouldClose(app_data.window_state.window_handle)) {
            glfwPollEvents();
            backend.new_frame(app_data);
            update();
            backend.step(app_data);
            backend.end_frame(app_data.window_state.window_handle);
            glfwSwapBuffers(app_data.window_state.window_handle);
        }
    }

    template <typename T>
    void finish(T& backend) {
        backend.finish();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(app_data.window_state.window_handle);
        glfwTerminate();    
    }
    
    void update();

};

#endif