#include <glm.hpp>
#include <stb_image.h>

#include <logger.hpp>
#include <object.hpp>
#include <shader.hpp>
#include <texture.hpp>
#include <window.hpp>

#include <format>
#include <iostream>

namespace rose {

float delta_time = 0.0f;
float last_frame_time = 0.0f;

glm::vec3 light_pos = { 1.2f, 1.0f, 2.0f };

static unsigned int VBO{};
static unsigned int object_VAO{};
static unsigned int light_VAO{};

bool WindowGLFW::init() {

    if (!glfwInit()) {
        LOG_ERROR("GLFW failed to initialize");
        return false;
    }

    LOG_INFO("GLFW successfully initialized version: {}", glfwGetVersionString());

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, name.c_str(), NULL, NULL);

    if (window == nullptr) {
        LOG_ERROR("Failed to create GLFW window");
        glfwTerminate();
        return false;
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
        LOG_ERROR("Glew failed to initialize: {}", (const char*)glewGetErrorString(glew_success));
        return false;
    }

    LOG_INFO("Glew successfully initialized version: {}", (const char*)glewGetString(GLEW_VERSION));

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);

    Cube cube{};

    glGenVertexArrays(1, &object_VAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, cube.size(), cube.verts.data(), GL_STATIC_DRAW);

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
        LOG_ERROR("Unable to load light object shader");
    }

    if (!light_source_shader.init(std::format("{}/rose/shaders/light_source.vert", SOURCE_DIR),
                                  std::format("{}/rose/shaders/light_source.frag", SOURCE_DIR))) {
        LOG_ERROR("Unable to load light source shader");
    }

    stbi_set_flip_vertically_on_load(true);

    // Load and create texture

    std::optional<unsigned int> diffuse_map = load_texture(std::format("{}/textures/diffuse_map.png", SOURCE_DIR));
    if (diffuse_map) this->diffuse_map = diffuse_map.value();

    std::optional<unsigned int> specular_map = load_texture(std::format("{}/textures/specular_map.png", SOURCE_DIR));
    if (specular_map) this->specular_map = specular_map.value();

    std::optional<unsigned int> emission_map = load_texture(std::format("{}/textures/emission_map.jpg", SOURCE_DIR));
    if (emission_map) this->emission_map = emission_map.value();

    return true;
};

void WindowGLFW::update() {

    glm::vec3 cube_positions[] = { glm::vec3(0.0f, 0.0f, 0.0f),    glm::vec3(2.0f, 5.0f, -15.0f),
                                   glm::vec3(-1.5f, -2.2f, -2.5f), glm::vec3(-3.8f, -2.0f, -12.3f),
                                   glm::vec3(2.4f, -0.4f, -3.5f),  glm::vec3(-1.7f, 3.0f, -7.5f),
                                   glm::vec3(1.3f, -2.0f, -2.5f),  glm::vec3(1.5f, 2.0f, -2.5f),
                                   glm::vec3(1.5f, 0.2f, -1.5f),   glm::vec3(-1.3f, 1.0f, -1.5f) };

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
        light_object_shader.set_vec3("camera_pos", camera.position);

        // light properties
        light_object_shader.set_vec3("light.direction", camera.front); // Spotlight
        light_object_shader.set_float("light.cutoff", glm::cos(glm::radians(12.5f)));

        light_object_shader.set_vec3("light.ambient", glm::vec3(0.15f, 0.15f, 0.15f));
        light_object_shader.set_vec3("light.diffuse", glm::vec3(0.5f, 0.5f, 0.5f));
        light_object_shader.set_vec3("light.specular", glm::vec3(1.0f, 1.0f, 1.0f));
        light_object_shader.set_float("light.attn_const", 1.0f);
        light_object_shader.set_float("light.attn_lin", 0.09f);
        light_object_shader.set_float("light.attn_quads", 0.032f);

        // material properties
        light_object_shader.set_int("material.diffuse", 0);
        light_object_shader.set_int("material.specular", 1);
        light_object_shader.set_vec3("material.ambient", glm::vec3(1.0f, 0.5f, 0.31f));
        light_object_shader.set_float("material.shine_factor", 32.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuse_map);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specular_map);

        glBindVertexArray(object_VAO);

        for (unsigned int i = 0; i < 10; i++) {
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

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, light_pos);
        model = glm::scale(model, glm::vec3(0.2f));
        light_source_shader.set_mat4("model", model);

        glBindVertexArray(light_VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

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