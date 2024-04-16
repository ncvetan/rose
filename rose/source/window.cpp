#include <glm.hpp>
#include <stb_image.h>

#include <err.hpp>
#include <logger.hpp>
#include <model.hpp>
#include <window.hpp>

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
    GLenum glew_success = glewInit();

    if (glew_success != GLEW_OK) {
        return rses().gl(std::format("Glew failed to initialize: {}", (const char*)glewGetErrorString(glew_success)));
    }

    LOG_INFO("Glew successfully initialized version: {}", (const char*)glewGetString(GLEW_VERSION));

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);

    if (!light_object_shader.init(std::format("{}/rose/shaders/light_object.vert", SOURCE_DIR),
                                  std::format("{}/rose/shaders/light_object.frag", SOURCE_DIR))) {
        LOG_WARN("Unable to load light object shader");
    }

    if (!light_source_shader.init(std::format("{}/rose/shaders/light_source.vert", SOURCE_DIR),
                                  std::format("{}/rose/shaders/light_source.frag", SOURCE_DIR))) {
        LOG_WARN("Unable to load light source shader");
    }

    stbi_set_flip_vertically_on_load(true);

    Model model;
    std::optional<rses> err = model.load(std::format("{}/assets/model1/model1.obj", SOURCE_DIR));
    if (err) {
        return err->general("Unable to load model");
    }

    objects.push_back({ std::move(model), glm::vec3(0.0f, 0.0f, 0.0f) });
    cubes.push_back({ Cube(), glm::vec3(-2.0f, -3.0f, -3.0f) });
    cubes.push_back({ Cube(), glm::vec3(-2.0f, -3.0f, -5.0f) });
    pnt_lights.push_back({ Cube(), glm::vec3(3.0f, 3.0f, 3.0f) });
    pnt_lights.push_back({ Cube(), glm::vec3(3.0f, 3.0f, 5.0f) });
    return std::nullopt;
};

void WindowGLFW::update() {
    
    for (auto& model : objects) {
        translate(model.first, model.second);
    }

    for (auto& cube : cubes) {
        translate(cube.first, cube.second);
        scale(cube.first, 0.75f);
    }

    for (auto& light : pnt_lights) {
        translate(light.first, light.second);
        scale(light.first, 0.33f);
    }
    
    while (!glfwWindowShouldClose(window)) {

        float current_frame_time = static_cast<float>(glfwGetTime());
        delta_time = current_frame_time - last_frame_time;
        last_frame_time = current_frame_time;

        process_input(window, delta_time);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = camera.projection_matrix(static_cast<float>(width) / static_cast<float>(height));
        glm::mat4 view = camera.view_matrix();
        light_object_shader.set_mat4("projection", projection);
        light_source_shader.set_mat4("projection", projection);
        light_object_shader.set_mat4("view", view);
        light_source_shader.set_mat4("view", view);
        light_object_shader.set_vec3("view_pos", camera.position);

        light_object_shader.set_vec3("dir_light.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
        light_object_shader.set_vec3("dir_light.ambient", glm::vec3(0.05f, 0.05f, 0.05f));
        light_object_shader.set_vec3("dir_light.diffuse", glm::vec3(0.4f, 0.4f, 0.4f));
        light_object_shader.set_vec3("dir_light.specular", glm::vec3(0.5f, 0.5f, 0.5f));

        for (int i = 0; i < pnt_lights.size(); ++i) {
            light_object_shader.set_vec3(std::format("point_lights[{}].position", i), pnt_lights[i].second);
            light_object_shader.set_vec3(std::format("point_lights[{}].ambient", i), glm::vec3(0.05f, 0.05f, 0.05f));
            light_object_shader.set_vec3(std::format("point_lights[{}].diffuse", i), glm::vec3(0.8f, 0.8f, 0.8f));
            light_object_shader.set_vec3(std::format("point_lights[{}].specular", i), glm::vec3(1.0f, 1.0f, 1.0f));
            light_object_shader.set_float(std::format("point_lights[{}].attn_const", i), 1.0f);
            light_object_shader.set_float(std::format("point_lights[{}].attn_lin", i), 0.09f);
            light_object_shader.set_float(std::format("point_lights[{}].attn_quads", i), 0.032f);
        }

        // todo: remove
        light_object_shader.set_float("materials[0].shine_factor", 32.0f);

        for (auto& model : objects) {
            model.first.draw(light_object_shader);
        }

        for (auto& cube : cubes) {
            cube.first.draw(light_object_shader);
        }

        for (auto& light : pnt_lights) {
            light.first.draw(light_source_shader);
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