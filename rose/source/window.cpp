#include <glm.hpp>
#include <stb_image.h>

#include <err.hpp>
#include <logger.hpp>
#include <model.hpp>
#include <window.hpp>

#include <format>
#include <iostream>
#include <set>
#include <ranges>

namespace rose {

float delta_time = 0.0f;
float last_frame_time = 0.0f;

std::optional<rses> WindowGLFW::init() {

    // GLFW initialization
    if (glfwInit() == GLFW_FALSE) {
        return rses().gl("GLFW failed to initialize");
    }

    LOG_INFO("GLFW successfully initialized version: {}", glfwGetVersionString());

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

    // GLEW initialization
    GLenum glew_success = glewInit();

    if (glew_success != GLEW_OK) {
        return rses().gl(std::format("Glew failed to initialize: {}", (const char*)glewGetErrorString(glew_success)));
    }

    LOG_INFO("Glew successfully initialized version: {}", (const char*)glewGetString(GLEW_VERSION));

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glFrontFace(GL_CW);

    if (!object_shader.init(std::format("{}/rose/shaders/object.vert", SOURCE_DIR),
                            std::format("{}/rose/shaders/object.frag", SOURCE_DIR))) {
        LOG_WARN("Unable to load object shader");
    }

    if (!light_shader.init(std::format("{}/rose/shaders/light.vert", SOURCE_DIR),
                           std::format("{}/rose/shaders/light.frag", SOURCE_DIR))) {
        LOG_WARN("Unable to load light shader");
    }

    if (!single_col_shader.init(std::format("{}/rose/shaders/single_col.vert", SOURCE_DIR),
                                std::format("{}/rose/shaders/single_col.frag", SOURCE_DIR))) {
        LOG_WARN("Unable to single color shader");
    }

    if (!texture_shader.init(std::format("{}/rose/shaders/texture.vert", SOURCE_DIR),
                              std::format("{}/rose/shaders/texture.frag", SOURCE_DIR))) {
        LOG_WARN("Unable to load cube shader");
    }

    stbi_set_flip_vertically_on_load(true);

    std::optional<rses> es;
    // floor
    tex_cubes.push_back({ TexturedCube(), glm::vec3(0.0f, -1.0f, 0.0f) });
    if (es = tex_cubes.back().first.load(std::format("{}/assets/texture1.png", SOURCE_DIR))) {
        err::print(*es);
    }
    // cubes
    tex_cubes.push_back({ TexturedCube(), glm::vec3(2.0f, 0.0f, 5.0f) });
    if (es = tex_cubes.back().first.load(std::format("{}/assets/texture2.jpg", SOURCE_DIR))) {
        err::print(*es);
    }
    tex_cubes.push_back({ TexturedCube(), glm::vec3(2.0f, 0.0f, 2.0f) });
    if (es = tex_cubes.back().first.load(std::format("{}/assets/texture2.jpg", SOURCE_DIR))) {
        err::print(*es);
    }

    quads.push_back({ TexturedQuad(), glm::vec3(0.0f, 0.25f, 0.0f) });
    if (es = quads.back().first.load(std::format("{}/assets/window.png", SOURCE_DIR))) {
        err::print(*es);
    }

    quads.push_back({ TexturedQuad(), glm::vec3(0.5f, 0.25f, 0.5f) });
    if (es = quads.back().first.load(std::format("{}/assets/window.png", SOURCE_DIR))) {
        err::print(*es);
    }

    return std::nullopt;
};

void WindowGLFW::update() {

    std::set<std::pair<float, int>> ordered_idxs;

    while (!glfwWindowShouldClose(window)) {

        float current_frame_time = static_cast<float>(glfwGetTime());
        delta_time = current_frame_time - last_frame_time;
        last_frame_time = current_frame_time;

        process_input(window, delta_time);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glEnable(GL_CULL_FACE);

        glm::mat4 projection = camera.projection_matrix(static_cast<float>(width) / static_cast<float>(height));
        glm::mat4 view = camera.view_matrix();
        texture_shader.set_mat4("projection", projection);
        texture_shader.set_mat4("view", view);

        for (auto& [cube, pos] : tex_cubes) {
            translate(cube, pos);
            if (cube.id == 0) {  // floor
                scale(cube, { 20.0f, 1.0f, 20.0f });
            } 
            cube.draw(texture_shader);
            cube.reset();
        }
        
        glDisable(GL_CULL_FACE);  // don't face cull 2D objects since we can view both faces
        for (int i = 0; const auto& [quad, pos] : quads) {
            float distance = glm::distance(camera.position, pos);
            ordered_idxs.insert({ distance, i });
            i++;
        }

        for (auto& [dist, i] : std::ranges::reverse_view(ordered_idxs)) {
            auto& [quad, pos] = quads[i];
            translate(quad, pos);
            quad.draw(texture_shader);
            quad.reset();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
};

void WindowGLFW::destroy() {
    glfwDestroyWindow(window);
    glfwTerminate();
    window = nullptr;
};

void WindowGLFW::enable_vsync(bool enable) { (enable) ? glfwSwapInterval(1) : glfwSwapInterval(0); };

void framebuffer_size_callback(GLFWwindow* window, int width, int height) { glViewport(0, 0, width, height); }

void mouse_callback(GLFWwindow* window, double xpos_in, double ypos_in) {
    WindowGLFW* window_state = (WindowGLFW*)glfwGetWindowUserPointer(window);

    float xpos = static_cast<float>(xpos_in);
    float ypos = static_cast<float>(ypos_in);

    float xoffset = xpos - window_state->last_xy.x;
    float yoffset = window_state->last_xy.y - ypos;

    window_state->last_xy.x = xpos;
    window_state->last_xy.y = ypos;
    window_state->camera.process_mouse_movement(xoffset, yoffset);
}

void resize_callback(GLFWwindow* window, int width, int height) {
    WindowGLFW* window_state = (WindowGLFW*)glfwGetWindowUserPointer(window);
    window_state->width = width;
    window_state->height = height;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    WindowGLFW* window_state = (WindowGLFW*)glfwGetWindowUserPointer(window);
    window_state->camera.process_mouse_scroll(static_cast<float>(yoffset));
}

void process_input(GLFWwindow* window, float delta_time) {
    WindowGLFW* window_state = (WindowGLFW*)glfwGetWindowUserPointer(window);
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
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}
} // namespace rose