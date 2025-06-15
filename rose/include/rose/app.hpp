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
    AppState app_state;

    template <typename T>
    rses init(T& backend) {
        rses err;
        app_state.init();
        if (err = init_glfw(app_state.window_state)) {
            return err.general("unable to initialize GLFW");
        }
        if (err = init_imgui(app_state.window_state)) {
            return err.general("unable to initialize Dear ImGui");
        }
        if (err = backend.init(app_state)) {
            return err.general("unable to initialize application");
        }
        return {};
    }
    
    template <typename T>
    void run(T& backend) {
        while (!glfwWindowShouldClose(app_state.window_state.window_handle)) {
            glfwPollEvents();
            backend.new_frame(app_state);
            update();
            backend.step(app_state);
            backend.end_frame(app_state.window_state.window_handle);
            glfwSwapBuffers(app_state.window_state.window_handle);
        }
    }

    template <typename T>
    void finish(T& backend) {
        backend.finish();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(app_state.window_state.window_handle);
        glfwTerminate();    
    }
    
    void update();

};

#endif