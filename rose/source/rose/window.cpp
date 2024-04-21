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

    if (auto es = shaders["quad"].init(SOURCE_DIR"/rose/shaders/gl/quad.vert",
                                       SOURCE_DIR"/rose/shaders/gl/quad.frag")) {
        err::print(*es);
    }

    if (auto es = shaders["texture"].init(SOURCE_DIR"/rose/shaders/gl/texture.vert",
                                          SOURCE_DIR"/rose/shaders/gl/texture.frag")) {
        err::print(*es);
    }

    if (auto es = shaders["skybox"].init(SOURCE_DIR"/rose/shaders/gl/skybox.vert",
                                         SOURCE_DIR"/rose/shaders/gl/skybox.frag"); es.has_value()) {
        err::print(*es);
    }

    std::optional<rses> es;
    
    // skybox
    sky_box.init();
    if (es = sky_box.load({ SOURCE_DIR"/assets/skybox/right.jpg", SOURCE_DIR"/assets/skybox/left.jpg", 
                            SOURCE_DIR"/assets/skybox/top.jpg", SOURCE_DIR"/assets/skybox/bottom.jpg",
                            SOURCE_DIR"/assets/skybox/front.jpg", SOURCE_DIR"/assets/skybox/back.jpg" })) {
        err::print(*es);
    }
    
    // floor
    tex_cubes.push_back(Object<TexturedCube>({ 0.0f, -1.0f, 0.0f }));
    tex_cubes.back().object.init();
    if (es = tex_cubes.back().object.load(SOURCE_DIR"/assets/texture1.png")) {
        err::print(*es);
    }
    // cubes
    tex_cubes.push_back(Object<TexturedCube>({ 2.0f, 0.0f, 5.0f }));
    tex_cubes.back().object.init();
    if (es = tex_cubes.back().object.load(SOURCE_DIR"/assets/texture2.jpg")) {
        err::print(*es);
    }
    tex_cubes.push_back(Object<TexturedCube>({ 2.0f, 0.0f, 2.0f }));
    tex_cubes.back().object.init();
    if (es = tex_cubes.back().object.load(SOURCE_DIR"/assets/texture2.jpg")) {
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

        for (auto& [name, shader] : shaders) {
            shader.set_mat4("projection", projection);
            shader.set_mat4("view", view);
            if (name == "skybox") {
                // removing translation component
                glm::mat4 static_view = view;
                static_view[3] = glm::vec4(0, 0, 0, 1);
                shader.set_mat4("view", static_view);
            }
        }

        // draw calls
        glDepthFunc(GL_LEQUAL);
        sky_box.draw(shaders["skybox"]);
        glDepthFunc(GL_LESS);

        for (auto& [cube, pos] : tex_cubes) {
            translate(cube, pos);
            if (cube.id == 2) { // floor
                scale(cube, { 20.0f, 1.0f, 20.0f });
            }
            cube.draw(shaders["texture"]);
            cube.reset();
        }

        // second pass
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);
        fbo_quad.draw(shaders["quad"]);

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