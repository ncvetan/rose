#include <glm.hpp>
#include <stb_image.h>

#include <logger.hpp>
#include <shader.hpp>
#include <window.hpp>

namespace rose
{
    float LAST_X = 400, LAST_Y = 300;
    float delta_time = 0.0f;
    float last_frame = 0.0f;

    static float vertices[] = {
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.5f,  -0.5f, -0.5f, 1.0f, 0.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
        0.5f,  0.5f,  -0.5f, 1.0f, 1.0f, -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,

        -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, 0.5f,  -0.5f, 0.5f,  1.0f, 0.0f, 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 1.0f, -0.5f, 0.5f,  0.5f,  0.0f, 1.0f, -0.5f, -0.5f, 0.5f,  0.0f, 0.0f,

        -0.5f, 0.5f,  0.5f,  1.0f, 0.0f, -0.5f, 0.5f,  -0.5f, 1.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, -0.5f, 0.5f,  0.5f,  1.0f, 0.0f,

        0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f, 0.5f,  -0.5f, -0.5f, 0.0f, 1.0f,
        0.5f,  -0.5f, -0.5f, 0.0f, 1.0f, 0.5f,  -0.5f, 0.5f,  0.0f, 0.0f, 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.5f,  -0.5f, -0.5f, 1.0f, 1.0f, 0.5f,  -0.5f, 0.5f,  1.0f, 0.0f,
        0.5f,  -0.5f, 0.5f,  1.0f, 0.0f, -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,

        -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f, 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f, -0.5f, 0.5f,  0.5f,  0.0f, 0.0f, -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f
    };

    static glm::vec3 cube_positions[] = { glm::vec3(0.0f, 0.0f, 0.0f),    glm::vec3(2.0f, 5.0f, -15.0f),
                                          glm::vec3(-1.5f, -2.2f, -2.5f), glm::vec3(-3.8f, -2.0f, -12.3f),
                                          glm::vec3(2.4f, -0.4f, -3.5f),  glm::vec3(-1.7f, 3.0f, -7.5f),
                                          glm::vec3(1.3f, -2.0f, -2.5f),  glm::vec3(1.5f, 2.0f, -2.5f),
                                          glm::vec3(1.5f, 0.2f, -1.5f),   glm::vec3(-1.3f, 1.0f, -1.5f) };

    static unsigned int VBO, VAO;
    static unsigned int texture1, texture2;

    bool WindowGLFW::init()
    {
        int init_success = glfwInit();
        if (init_success == GLFW_FALSE)
        {
            LOG_ERROR("GLFW failed to initialize");
            return false;
        }
        std::string glfw_v_str = glfwGetVersionString();
        LOG_INFO("GLFW successfully initialized version: {}", glfw_v_str);

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(width, height, name.c_str(), NULL, NULL);

        if (window == nullptr)
        {
            LOG_ERROR("Failed to create GLFW window");
            glfwTerminate();
        }
        LOG_INFO("GLFW window has been successfully created");

        glfwMakeContextCurrent(window);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetCursorPosCallback(window, mouse_callback);
        glfwSetScrollCallback(window, scroll_callback);

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // GLEW initialization
        GLenum glew_success = glewInit();
        if (glew_success == GLEW_OK)
        {
            std::string_view glew_v_str = (const char*)glewGetString(GLEW_VERSION);
            LOG_INFO("Glew successfully initialized version: {}", glew_v_str);
        }
        else
        {
            std::string_view glew_err_str = (const char*)glewGetErrorString(glew_success);
            LOG_ERROR("Glew failed to initialize: {}", glew_err_str);
            return false;
        }

        // ..................................

        glViewport(0, 0, 1280, 720);
        glEnable(GL_DEPTH_TEST);

        shader.init("./shaders/vertex.vert", "./shaders/fragment.frag");

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // texture coord attribute
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        stbi_set_flip_vertically_on_load(true);

        // Load and create texture

        glGenTextures(1, &texture1);
        glBindTexture(GL_TEXTURE_2D, texture1);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        int width, height, nr_channels;
        unsigned char* data = stbi_load("./textures/container.jpg", &width, &height, &nr_channels, 0);

        if (data)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        else
        {
            LOG_ERROR("Failed to load texture");
        }

        stbi_image_free(data);

        glGenTextures(1, &texture2);
        glBindTexture(GL_TEXTURE_2D, texture2);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        data = stbi_load("./textures/awesomeface.png", &width, &height, &nr_channels, 0);

        if (data)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        else
        {
            LOG_ERROR("Failed to load texture");
        }

        stbi_image_free(data);

        shader.use();
        shader.set_int("texture1", 0);
        shader.set_int("texture2", 1);

        camera.update_projection_mat();
        shader.set_mat4("projection", camera.projection);

        return true;
    };

    void WindowGLFW::update()
    {
        while (!glfwWindowShouldClose(window))
        {
            glClearColor(0.95f, 0.23f, 0.42f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            float current_frame = static_cast<float>(glfwGetTime());
            delta_time = current_frame - last_frame;
            last_frame = current_frame;

            process_input(window, delta_time);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, texture2);

            shader.use();

            shader.set_mat4("projection", camera.projection);

            glm::mat4 view = camera.get_view_matrix();
            shader.set_mat4("view", view);

            // Render container
            glBindVertexArray(VAO);

            for (unsigned int i = 0; i < 10; ++i)
            {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, cube_positions[i]);
                float angle = 20.0f * i;
                model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }

            // Swap buffers and poll IO events
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    };

    void WindowGLFW::destroy()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        window = nullptr;

        // Deallocating resources
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    };

    double WindowGLFW::get_time() { return glfwGetTime(); };

    void WindowGLFW::enable_vsync(bool enable)
    {
        if (enable)
            glfwSwapInterval(1);
        else
            glfwSwapInterval(0);
    };

    void framebuffer_size_callback(GLFWwindow* window, int width, int height) { glViewport(0, 0, width, height); }

    void mouse_callback(GLFWwindow* window, double xpos_in, double ypos_in)
    {
        float xpos = static_cast<float>(xpos_in);
        float ypos = static_cast<float>(ypos_in);

        float xoffset = xpos - LAST_X;
        float yoffset = LAST_Y - ypos;

        LAST_X = xpos;
        LAST_Y = ypos;

        camera.process_mouse_movement(xoffset, yoffset);
    }

    void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
    {
        camera.process_mouse_scroll(static_cast<float>(yoffset));
    }

    void process_input(GLFWwindow* window, float delta_time)
    {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            camera.process_keyboard(CameraMovement::FORWARD, delta_time);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            camera.process_keyboard(CameraMovement::BACKWARD, delta_time);
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            camera.process_keyboard(CameraMovement::LEFT, delta_time);
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            camera.process_keyboard(CameraMovement::RIGHT, delta_time);
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        {
            camera.process_keyboard(CameraMovement::DOWN, delta_time);
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            camera.process_keyboard(CameraMovement::UP, delta_time);
        }
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }
    }
} // namespace rose