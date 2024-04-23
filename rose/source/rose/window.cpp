#include <rose/err.hpp>
#include <rose/gui.hpp>
#include <rose/logger.hpp>
#include <rose/model.hpp>
#include <rose/window.hpp>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <stb_image.h>

#include <format>
#include <iostream>

namespace rose {

std::optional<rses> WindowGLFW::init() {

    // GLFW initialization
    if (glfwInit() == GLFW_FALSE) {
        return rses().gl("GLFW failed to initialize");
    }

    LOG_INFO("GLFW successfully initialized version: {}", glfwGetVersionString());

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4); // MSAA

    window = glfwCreateWindow(width, height, name.c_str(), NULL, NULL);

    if (!window) {
        glfwTerminate();
        return rses().gl("Failed to create GLFW window");
    }

    LOG_INFO("GLFW window has been successfully created");

    glfwSetWindowUserPointer(window, this);
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeCallback(window, resize_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // ImGui initialization
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

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    // GLEW initialization
    if (GLenum glew_success = glewInit(); glew_success != GLEW_OK) {
        return rses().gl(std::format("Glew failed to initialize: {}", (const char*)glewGetErrorString(glew_success)));
    }

    LOG_INFO("Glew successfully initialized version: {}", (const char*)glewGetString(GLEW_VERSION));

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_MULTISAMPLE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (auto es =
            shaders["quad"].init(SOURCE_DIR "/rose/shaders/gl/quad.vert", SOURCE_DIR "/rose/shaders/gl/quad.frag")) {
        err::print(*es);
    }

    if (auto es = shaders["texture"].init(SOURCE_DIR "/rose/shaders/gl/texture.vert",
                                          SOURCE_DIR "/rose/shaders/gl/texture.frag")) {
        err::print(*es);
    }

    if (auto es = shaders["skybox"].init(SOURCE_DIR "/rose/shaders/gl/skybox.vert",
                                         SOURCE_DIR "/rose/shaders/gl/skybox.frag")) {
        err::print(*es);
    }

    if (auto es = shaders["skybox_reflect"].init(SOURCE_DIR "/rose/shaders/gl/skybox_reflect.vert",
                                                 SOURCE_DIR "/rose/shaders/gl/skybox_reflect.frag")) {
        err::print(*es);
    }

    if (auto es = shaders["object"].init(SOURCE_DIR "/rose/shaders/gl/object.vert",
                                         SOURCE_DIR "/rose/shaders/gl/object.frag")) {
        err::print(*es);
    }

    std::optional<rses> es;

    // skybox
    state.sky_box.init();
    if (es = state.sky_box.load({ SOURCE_DIR "/assets/skybox/right.jpg", SOURCE_DIR "/assets/skybox/left.jpg",
                                  SOURCE_DIR "/assets/skybox/top.jpg", SOURCE_DIR "/assets/skybox/bottom.jpg",
                                  SOURCE_DIR "/assets/skybox/front.jpg", SOURCE_DIR "/assets/skybox/back.jpg" })) {
        err::print(*es);
    }

    // floor
    tex_cubes.push_back(Object<TexturedCube>({ 0.0f, -1.0f, 0.0f }, { 20.0f, 1.0f, 20.0f }));
    tex_cubes.back().object.init();
    if (es = tex_cubes.back().object.load(SOURCE_DIR "/assets/texture1.png")) {
        err::print(*es);
    }

    // cube
    tex_cubes.push_back(Object<TexturedCube>({ 2.0f, 0.0f, 5.0f }));
    tex_cubes.back().object.init();
    if (es = tex_cubes.back().object.load(SOURCE_DIR "/assets/texture2.jpg")) {
        err::print(*es);
    }

    // framebuffer
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    fbo_quad.init();
    fbo_quad.texture = *generate_texture(width, height);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_quad.texture.ref->id, 0);
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return rses().gl("framebuffer is incomplete");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // uniform buffer
    glGenBuffers(1, &state.ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, state.ubo);
    glBufferData(GL_UNIFORM_BUFFER, 208, nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, state.ubo);

    return std::nullopt;
};

void WindowGLFW::update() {

    state.dir_light.direction = { 0.0f, -1.0f, 0.0f };
    state.dir_light.ambient = { 0.1f, 0.1f, 0.1f };
    state.dir_light.diffuse = { 0.3f, 0.3f, 0.3f };
    state.dir_light.specular = { 1.0f, 1.0f, 1.0f };

    while (!glfwWindowShouldClose(window)) {

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        dock_id = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        ImGuiIO& io = ImGui::GetIO();
        (void)io;

        process_input(window, io.DeltaTime);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        glm::mat4 projection = camera.projection_matrix(static_cast<float>(width) / static_cast<float>(height));
        glm::mat4 view = camera.view_matrix();

        // update ubo state
        glBindBuffer(GL_UNIFORM_BUFFER, state.ubo);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection));
        glBufferSubData(GL_UNIFORM_BUFFER, 128, 16, glm::value_ptr(camera.position));
        glBufferSubData(GL_UNIFORM_BUFFER, 144, 16, glm::value_ptr(state.dir_light.direction));
        glBufferSubData(GL_UNIFORM_BUFFER, 160, 16, glm::value_ptr(state.dir_light.ambient));
        glBufferSubData(GL_UNIFORM_BUFFER, 176, 16, glm::value_ptr(state.dir_light.diffuse));
        glBufferSubData(GL_UNIFORM_BUFFER, 192, 16, glm::value_ptr(state.dir_light.specular));

        // draw calls
        glm::mat4 static_view = view;
        static_view[3] = glm::vec4(0, 0, 0, 1); // removing translation component
        glBufferSubData(GL_UNIFORM_BUFFER, 64, sizeof(glm::mat4), glm::value_ptr(static_view));
        state.sky_box.draw(shaders["skybox"], state);

        glBufferSubData(GL_UNIFORM_BUFFER, 64, sizeof(glm::mat4), glm::value_ptr(view));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        for (auto& [cube, pos_vec, scale_vec] : tex_cubes) {
            translate(cube, pos_vec);
            scale(cube, scale_vec);
            cube.draw(shaders["object"], state);
            cube.reset();
        }

        // second pass
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glBindTexture(GL_TEXTURE_2D, fbo_quad.texture.ref->id);

        gui::imgui(*this);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
};

void WindowGLFW::destroy() {
    glDeleteBuffers(1, &fbo);
    glDeleteBuffers(1, &rbo);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    window = nullptr;
};

void WindowGLFW::enable_vsync(bool enable) { (enable) ? glfwSwapInterval(1) : glfwSwapInterval(0); };

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    WindowGLFW* window_state = (WindowGLFW*)glfwGetWindowUserPointer(window);

    // handle window resizing
    glBindTexture(GL_TEXTURE_2D, window_state->fbo_quad.texture.ref->id);
    glBindFramebuffer(1, window_state->fbo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, window_state->fbo_quad.texture.ref->id,
                           0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, window_state->rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, window_state->rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos_in, double ypos_in) {
    WindowGLFW* window_state = (WindowGLFW*)glfwGetWindowUserPointer(window);
    window_state->mouse_xy = { (float)xpos_in, (float)ypos_in };
    if (window_state->glfw_captured) {
        float xpos = static_cast<float>(xpos_in);
        float ypos = static_cast<float>(ypos_in);
        float xoffset = xpos - window_state->last_xy.x;
        float yoffset = window_state->last_xy.y - ypos;
        window_state->last_xy.x = xpos;
        window_state->last_xy.y = ypos;
        window_state->camera.process_mouse_movement(xoffset, yoffset);
    }
}

void resize_callback(GLFWwindow* window, int width, int height) {
    WindowGLFW* window_state = (WindowGLFW*)glfwGetWindowUserPointer(window);
    window_state->width = width;
    window_state->height = height;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    WindowGLFW* window_state = (WindowGLFW*)glfwGetWindowUserPointer(window);
    if (window_state->glfw_captured) {
        window_state->camera.process_mouse_scroll(static_cast<float>(yoffset));
    }
}

void process_input(GLFWwindow* window, float delta_time) {
    WindowGLFW* window_state = (WindowGLFW*)glfwGetWindowUserPointer(window);
    if (window_state->glfw_captured) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            window_state->camera.process_keyboard(CameraMovement::FORWARD, delta_time);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            window_state->camera.process_keyboard(CameraMovement::BACKWARD, delta_time);
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            window_state->camera.process_keyboard(CameraMovement::LEFT, delta_time);
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            window_state->camera.process_keyboard(CameraMovement::RIGHT, delta_time);
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            window_state->camera.process_keyboard(CameraMovement::DOWN, delta_time);
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            window_state->camera.process_keyboard(CameraMovement::UP, delta_time);
        }
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (window_state->vp_sz.contains(window_state->mouse_xy.x, window_state->mouse_xy.y)) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            window_state->glfw_captured = true;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        window_state->glfw_captured = false;
    }
}
} // namespace rose