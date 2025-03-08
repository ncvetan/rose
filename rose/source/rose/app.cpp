#include <rose/app.hpp>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>

#ifdef OPENGL
#include <backends/imgui_impl_opengl3.h>
#endif

namespace rose {

std::optional<rses> init_glfw(WindowData& window_data) {
    if (glfwInit() == GLFW_FALSE) {
        return rses().gl("GLFW failed to initialize");
    }

    std::println("GLFW successfully initialized version: {}", glfwGetVersionString());

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

#ifdef _DEBUG
    glfwWindowHint(GLFW_CONTEXT_DEBUG, GL_TRUE);
#endif

    window_data.window_handle = glfwCreateWindow(window_data.width, window_data.height,
                                          window_data.name.c_str(), NULL, NULL);

    if (!window_data.window_handle) {
        glfwTerminate();
        return rses().gl("Failed to create GLFW window");
    }

    std::println("GLFW window has been successfully created");

    glfwSetWindowUserPointer(window_data.window_handle, &window_data);
    glfwMakeContextCurrent(window_data.window_handle);
    glfwSetInputMode(window_data.window_handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    return std::nullopt;
}

std::optional<rses> init_imgui(WindowData& window_data) {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForOpenGL(window_data.window_handle, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    return std::nullopt;
}

void RoseApp::handle_events() {

    ImGuiIO& io = ImGui::GetIO();
    app_data.window_data.mouse_xy = { io.MousePos.x, io.MousePos.y };

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (app_data.window_data.vp_rect.contains(app_data.window_data.mouse_xy) && app_data.window_data.vp_focused) {
            app_data.window_data.vp_captured = true;
            glfwSetInputMode(app_data.window_data.window_handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        app_data.window_data.vp_captured = false;
        glfwSetInputMode(app_data.window_data.window_handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    if (app_data.window_data.vp_captured) {
        if (app_data.window_data.last_xy != app_data.window_data.mouse_xy) {
            f32 xoffset = app_data.window_data.mouse_xy.x - app_data.window_data.last_xy.x;
            f32 yoffset = app_data.window_data.last_xy.y - app_data.window_data.mouse_xy.y;
            app_data.camera.handle_mouse(xoffset, yoffset);
            app_data.window_data.last_xy = app_data.window_data.mouse_xy;
        }

        f32 delta_time = io.DeltaTime;

        if (ImGui::IsKeyDown(ImGuiKey_W)) {
            app_data.camera.handle_keyboard(CameraMovement::FORWARD, delta_time);
        }
        if (ImGui::IsKeyDown(ImGuiKey_A)) {
            app_data.camera.handle_keyboard(CameraMovement::LEFT, delta_time);
        }
        if (ImGui::IsKeyDown(ImGuiKey_S)) {
            app_data.camera.handle_keyboard(CameraMovement::BACKWARD, delta_time);
        }
        if (ImGui::IsKeyDown(ImGuiKey_D)) {
            app_data.camera.handle_keyboard(CameraMovement::RIGHT, delta_time);
        }
        if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
            app_data.camera.handle_keyboard(CameraMovement::DOWN, delta_time);
        }
        if (ImGui::IsKeyDown(ImGuiKey_Space)) {
            app_data.camera.handle_keyboard(CameraMovement::UP, delta_time);
        }
    }
}

} // namespace rose
