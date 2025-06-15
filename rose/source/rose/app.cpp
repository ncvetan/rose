#include <rose/app.hpp>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>

#ifdef USE_OPENGL
#include <backends/imgui_impl_opengl3.h>
#else
static_assert("no backend selected");
#endif

rses init_glfw(WindowState& window_state) {
    if (glfwInit() == GLFW_FALSE) {
        return rses().gl("GLFW failed to initialize");
    }

    std::println("GLFW successfully initialized version: {}", glfwGetVersionString());

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    glfwSwapInterval(1); // Enable vsync

#ifdef _DEBUG
    glfwWindowHint(GLFW_CONTEXT_DEBUG, GL_TRUE);
#endif

    window_state.window_handle = glfwCreateWindow(window_state.width, window_state.height,
                                          window_state.name.c_str(), NULL, NULL);

    if (!window_state.window_handle) {
        glfwTerminate();
        return rses().gl("Failed to create GLFW window");
    }

    std::println("GLFW window has been successfully created");

    glfwSetWindowUserPointer(window_state.window_handle, &window_state);
    glfwMakeContextCurrent(window_state.window_handle);
    glfwSetInputMode(window_state.window_handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    return {};
}

namespace pallette {
ImVec4 red_active = { 0.886f, 0.427f, 0.353f, 1.000f };
ImVec4 red_inactive = { 0.886f, 0.427f, 0.353f, 0.800f };
ImVec4 yellow_active = { 1.000f, 0.784f, 0.341f, 1.000f };
ImVec4 yellow_inactive = { 1.000f, 0.784f, 0.341f, 0.800f };
ImVec4 light_purple_active = { 0.486f, 0.447f, 0.627f, 1.000f };
ImVec4 mid_purple_active = { 0.274f, 0.251f, 0.353f, 1.000f };
ImVec4 mid_purple_inactive = { 0.274f, 0.251f, 0.353f, 0.800f };
ImVec4 deep_purple_active = { 0.141f, 0.145f, 0.173f, 1.000f };
ImVec4 deep_purple_inactive = { 0.141f, 0.145f, 0.173f, 1.000f };
}

static void set_imgui_theme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text] = pallette::yellow_active;
    colors[ImGuiCol_TextDisabled] = pallette::yellow_inactive;
    colors[ImGuiCol_WindowBg] = pallette::deep_purple_active;

    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.11f, 0.14f, 0.92f);
    colors[ImGuiCol_Border] = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.43f, 0.43f, 0.43f, 0.39f);

    colors[ImGuiCol_FrameBgHovered] = pallette::mid_purple_inactive;
    colors[ImGuiCol_FrameBgActive] = pallette::mid_purple_active;
    colors[ImGuiCol_TitleBg] = pallette::mid_purple_inactive;
    colors[ImGuiCol_TitleBgActive] = pallette::mid_purple_active;
    colors[ImGuiCol_TitleBgCollapsed] = pallette::mid_purple_inactive;
    colors[ImGuiCol_MenuBarBg] = pallette::mid_purple_active;
    colors[ImGuiCol_ScrollbarBg] = pallette::mid_purple_inactive;

    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.40f, 0.40f, 0.80f, 0.30f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.80f, 0.40f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
    colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);

    colors[ImGuiCol_Button] = pallette::mid_purple_inactive;
    colors[ImGuiCol_ButtonHovered] = pallette::mid_purple_active;
    colors[ImGuiCol_ButtonActive] = pallette::mid_purple_active;
    colors[ImGuiCol_Header] = pallette::mid_purple_active;
    colors[ImGuiCol_HeaderHovered] = pallette::mid_purple_active;
    colors[ImGuiCol_HeaderActive] = pallette::light_purple_active;

    colors[ImGuiCol_Separator] = ImVec4(0.50f, 0.50f, 0.50f, 0.60f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.60f, 0.60f, 0.70f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.70f, 0.70f, 0.90f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.16f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.78f, 0.82f, 1.00f, 0.60f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.78f, 0.82f, 1.00f, 0.90f);

    colors[ImGuiCol_Tab] = pallette::mid_purple_active;
    colors[ImGuiCol_TabHovered] = pallette::mid_purple_active;
    colors[ImGuiCol_TabActive] = pallette::mid_purple_active;
    colors[ImGuiCol_TabUnfocused] = pallette::mid_purple_active;
    colors[ImGuiCol_TabUnfocusedActive] = pallette::mid_purple_active;

    colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}


rses init_imgui(WindowState& window_state) {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    set_imgui_theme();

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

#ifdef USE_OPENGL
    ImGui_ImplGlfw_InitForOpenGL(window_state.window_handle, true);
    ImGui_ImplOpenGL3_Init("#version 460");
#endif

    return {};
}

void RoseApp::update() {

    ImGuiIO& io = ImGui::GetIO();
    app_state.window_state.mouse_xy = { io.MousePos.x, io.MousePos.y };

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (app_state.window_state.vp_rect.contains(app_state.window_state.mouse_xy) && app_state.window_state.vp_focused) {
            app_state.window_state.vp_captured = true;
            glfwSetInputMode(app_state.window_state.window_handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        app_state.window_state.vp_captured = false;
        glfwSetInputMode(app_state.window_state.window_handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    if (app_state.window_state.vp_captured) {
        if (app_state.window_state.last_xy != app_state.window_state.mouse_xy) {
            f32 xoffset = app_state.window_state.mouse_xy.x - app_state.window_state.last_xy.x;
            f32 yoffset = app_state.window_state.last_xy.y - app_state.window_state.mouse_xy.y;
            app_state.camera.handle_mouse(xoffset, yoffset);
            app_state.window_state.last_xy = app_state.window_state.mouse_xy;
        }

        f32 delta_time = io.DeltaTime;

        if (ImGui::IsKeyDown(ImGuiKey_W)) {
            app_state.camera.handle_keyboard(CameraMovement::FORWARD, delta_time);
        }
        if (ImGui::IsKeyDown(ImGuiKey_A)) {
            app_state.camera.handle_keyboard(CameraMovement::LEFT, delta_time);
        }
        if (ImGui::IsKeyDown(ImGuiKey_S)) {
            app_state.camera.handle_keyboard(CameraMovement::BACKWARD, delta_time);
        }
        if (ImGui::IsKeyDown(ImGuiKey_D)) {
            app_state.camera.handle_keyboard(CameraMovement::RIGHT, delta_time);
        }
        if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
            app_state.camera.handle_keyboard(CameraMovement::DOWN, delta_time);
        }
        if (ImGui::IsKeyDown(ImGuiKey_Space)) {
            app_state.camera.handle_keyboard(CameraMovement::UP, delta_time);
        }
    }
}
