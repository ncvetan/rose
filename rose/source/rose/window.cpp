#include <rose/err.hpp>
#include <rose/logger.hpp>
#include <rose/model.hpp>
#include <rose/window.hpp>

#include <glm.hpp>
#include <stb_image.h>

#include <format>
#include <iostream>

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
    if (GLenum glew_success = glewInit(); glew_success != GLEW_OK) {
        return rses().gl(std::format("Glew failed to initialize: {}", (const char*)glewGetErrorString(glew_success)));
    }

    LOG_INFO("Glew successfully initialized version: {}", (const char*)glewGetString(GLEW_VERSION));

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST | GL_STENCIL_TEST | GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (auto es = light_shader.init(std::format("{}/rose/shaders/gl/light.vert", SOURCE_DIR),
                                    std::format("{}/rose/shaders/gl/light.frag", SOURCE_DIR))) {
        err::print(*es);
    }

    if (auto es = object_shader.init(std::format("{}/rose/shaders/gl/object.vert", SOURCE_DIR),
                                     std::format("{}/rose/shaders/gl/object.frag", SOURCE_DIR))) {
        err::print(*es);
    }

    if (auto es = quad_shader.init(std::format("{}/rose/shaders/gl/quad.vert", SOURCE_DIR),
                                   std::format("{}/rose/shaders/gl/quad.frag", SOURCE_DIR))) {
        err::print(*es);
    }

    if (auto es = texture_shader.init(std::format("{}/rose/shaders/gl/texture.vert", SOURCE_DIR),
                                      std::format("{}/rose/shaders/gl/texture.frag", SOURCE_DIR))) {
        err::print(*es);
    }

    if (auto es = single_col_shader.init(std::format("{}/rose/shaders/gl/single_col.vert", SOURCE_DIR),
                                         std::format("{}/rose/shaders/gl/single_col.frag", SOURCE_DIR))) {
        err::print(*es);
    }

    stbi_set_flip_vertically_on_load(true);

    std::optional<rses> es;
    // floor
    tex_cubes.push_back({ TexturedCube(), glm::vec3(0.0f, -1.0f, 0.0f) });
    tex_cubes.back().first.init();
    if (es = tex_cubes.back().first.load(std::format("{}/assets/texture1.png", SOURCE_DIR))) {
        err::print(*es);
    }
    // cubes
    tex_cubes.push_back({ TexturedCube(), glm::vec3(2.0f, 0.0f, 5.0f) });
    tex_cubes.back().first.init();
    if (es = tex_cubes.back().first.load(std::format("{}/assets/texture2.jpg", SOURCE_DIR))) {
        err::print(*es);
    }
    tex_cubes.push_back({ TexturedCube(), glm::vec3(2.0f, 0.0f, 2.0f) });
    tex_cubes.back().first.init();
    if (es = tex_cubes.back().first.load(std::format("{}/assets/texture2.jpg", SOURCE_DIR))) {
        err::print(*es);
    }

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
    return std::nullopt;
};

void WindowGLFW::update() {

    while (!glfwWindowShouldClose(window)) {

        float current_frame_time = static_cast<float>(glfwGetTime());
        delta_time = current_frame_time - last_frame_time;
        last_frame_time = current_frame_time;
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        process_input(window, delta_time);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        glm::mat4 projection = camera.projection_matrix(static_cast<float>(width) / static_cast<float>(height));
        glm::mat4 view = camera.view_matrix();
        texture_shader.set_mat4("projection", projection);
        texture_shader.set_mat4("view", view);

        for (auto& [cube, pos] : tex_cubes) {
            translate(cube, pos);
            if (cube.id == 1) { // floor
                scale(cube, { 20.0f, 1.0f, 20.0f });
            }
            cube.draw(texture_shader);
            cube.reset();
        }

        // second pass
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to default
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);
        fbo_quad.draw(quad_shader);

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