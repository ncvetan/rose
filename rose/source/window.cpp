#include <glm.hpp>
#include <stb_image.h>

#include <err.hpp>
#include <logger.hpp>
#include <mesh.hpp>
#include <shader.hpp>
#include <texture.hpp>
#include <window.hpp>

#include <format>
#include <iostream>

namespace rose {

float delta_time = 0.0f;
float last_frame_time = 0.0f;

static unsigned int VBO{};
static unsigned int object_VAO{};
static unsigned int light_VAO{};

std::optional<rses> WindowGLFW::init() {

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

    // todo: fix this code
    // Cube cube{};

    glGenVertexArrays(1, &object_VAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // glBufferData(GL_ARRAY_BUFFER, cube.size(), cube.verts.data(), GL_STATIC_DRAW);

    glBindVertexArray(object_VAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glGenVertexArrays(1, &light_VAO);
    glBindVertexArray(light_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    if (!light_object_shader.init(std::format("{}/rose/shaders/light_object.vert", SOURCE_DIR),
                                  std::format("{}/rose/shaders/light_object.frag", SOURCE_DIR))) {
        LOG_WARN("Unable to load light object shader");
    }

    if (!light_source_shader.init(std::format("{}/rose/shaders/light_source.vert", SOURCE_DIR),
                                  std::format("{}/rose/shaders/light_source.frag", SOURCE_DIR))) {
        LOG_WARN("Unable to load light source shader");
    }

    stbi_set_flip_vertically_on_load(true);

    // Load and create texture

    std::optional<unsigned int> diffuse_map = load_texture(std::format("{}/textures/diffuse_map.png", SOURCE_DIR));
    if (diffuse_map) this->diffuse_map = diffuse_map.value();

    std::optional<unsigned int> specular_map = load_texture(std::format("{}/textures/specular_map.png", SOURCE_DIR));
    if (specular_map) this->specular_map = specular_map.value();

    std::optional<unsigned int> emission_map = load_texture(std::format("{}/textures/emission_map.jpg", SOURCE_DIR));
    if (emission_map) this->emission_map = emission_map.value();

    return std::nullopt;
};

void WindowGLFW::update() {

    glm::vec3 cube_positions[] = { glm::vec3(0.0f, 0.0f, 0.0f),    glm::vec3(2.0f, 5.0f, -15.0f),
                                   glm::vec3(-1.5f, -2.2f, -2.5f), glm::vec3(-3.8f, -2.0f, -12.3f),
                                   glm::vec3(2.4f, -0.4f, -3.5f),  glm::vec3(-1.7f, 3.0f, -7.5f),
                                   glm::vec3(1.3f, -2.0f, -2.5f),  glm::vec3(1.5f, 2.0f, -2.5f),
                                   glm::vec3(1.5f, 0.2f, -1.5f),   glm::vec3(-1.3f, 1.0f, -1.5f) };

    glm::vec3 point_light_positions[] = { glm::vec3(0.7f, 0.2f, 2.0f), glm::vec3(2.3f, -3.3f, -4.0f),
                                          glm::vec3(-4.0f, 2.0f, -12.0f), glm::vec3(0.0f, 0.0f, -3.0f) };

    while (!glfwWindowShouldClose(window)) {

        float current_frame_time = static_cast<float>(glfwGetTime());
        delta_time = current_frame_time - last_frame_time;
        last_frame_time = current_frame_time;

        process_input(window, delta_time);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        light_object_shader.use();
        glm::mat4 projection = camera.projection_matrix(static_cast<float>(width) / static_cast<float>(height));
        light_object_shader.set_mat4("projection", projection);
        glm::mat4 view = camera.view_matrix();
        light_object_shader.set_mat4("view", view);
        light_object_shader.set_vec3("view_pos", camera.position);

        light_object_shader.set_vec3("dir_light.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
        light_object_shader.set_vec3("dir_light.ambient", glm::vec3(0.05f, 0.05f, 0.05f));
        light_object_shader.set_vec3("dir_light.diffuse", glm::vec3(0.4f, 0.4f, 0.4f));
        light_object_shader.set_vec3("dir_light.specular", glm::vec3(0.5f, 0.5f, 0.5f));

        for (int i = 0; i < sizeof(point_light_positions) / sizeof(point_light_positions[i]); ++i) {
            light_object_shader.set_vec3(std::format("point_lights[{}].position", i), point_light_positions[i]);
            light_object_shader.set_vec3(std::format("point_lights[{}].ambient", i), glm::vec3(0.05f, 0.05f, 0.05f));
            light_object_shader.set_vec3(std::format("point_lights[{}].diffuse", i), glm::vec3(0.8f, 0.8f, 0.8f));
            light_object_shader.set_vec3(std::format("point_lights[{}].specular", i), glm::vec3(1.0f, 1.0f, 1.0f));
            light_object_shader.set_float(std::format("point_lights[{}].attn_const", i), 1.0f);
            light_object_shader.set_float(std::format("point_lights[{}].attn_lin", i), 0.09f);
            light_object_shader.set_float(std::format("point_lights[{}].attn_quads", i), 0.032f);
        }

        // light properties
        light_object_shader.set_vec3("spot_light.direction", camera.front);   // Spotlight
        light_object_shader.set_vec3("spot_light.position", camera.position); // Spotlight
        light_object_shader.set_float("spot_light.inner_cutoff", glm::cos(glm::radians(12.5f)));
        light_object_shader.set_float("spot_light.outer_cutoff", glm::cos(glm::radians(17.5f)));
        light_object_shader.set_vec3("spot_light.ambient", glm::vec3(0.0f, 0.0f, 0.0f));
        light_object_shader.set_vec3("spot_light.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));
        light_object_shader.set_vec3("spot_light.specular", glm::vec3(1.0f, 1.0f, 1.0f));
        light_object_shader.set_float("spot_light.attn_const", 1.0f);
        light_object_shader.set_float("spot_light.attn_lin", 0.09f);
        light_object_shader.set_float("spot_light.attn_quad", 0.032f);

        // material properties
        light_object_shader.set_int("material.diffuse_map", 0);
        light_object_shader.set_int("material.specular_map", 1);
        light_object_shader.set_float("material.shine_factor", 32.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuse_map);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specular_map);

        glBindVertexArray(object_VAO);

        for (int i = 0; i < 10; ++i) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cube_positions[i]);
            float angle = 20.0f * i;
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            light_object_shader.set_mat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        light_source_shader.use();
        light_source_shader.set_mat4("projection", projection);
        light_source_shader.set_mat4("view", view);

        glBindVertexArray(light_VAO);

        for (int i = 0; i < 4; ++i) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, point_light_positions[i]);
            model = glm::scale(model, glm::vec3(0.2f));
            light_source_shader.set_mat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
};

void WindowGLFW::destroy() {
    glfwDestroyWindow(window);
    glfwTerminate();
    window = nullptr;
    glDeleteVertexArrays(1, &object_VAO);
    glDeleteBuffers(1, &VBO);
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