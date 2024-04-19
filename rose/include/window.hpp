#ifndef ROSE_INCLUDE_WINDOW
#define ROSE_INCLUDE_WINDOW

#include <optional>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <camera.hpp>
#include <err.hpp>
#include <model.hpp>
#include <shader.hpp>

namespace rose {

template <typename T>
concept drawable = requires(T d, ShaderGL& shader) {
    { d.draw(shader) } -> std::same_as<void>;
};

class WindowGLFW {
  public:
    WindowGLFW() = default;

    std::optional<rses> init();
    void enable_vsync(bool enable);
    void update();
    void destroy();

    GLFWwindow* window = nullptr;
    CameraGL camera;
    
    std::vector<ShaderGL> shaders;
    std::unordered_map<std::string, int> shader_index;

    ShaderGL object_shader;
    ShaderGL light_shader;
    ShaderGL texture_shader;
    ShaderGL single_col_shader;
    
    std::vector<std::pair<Model, glm::vec3>> objects;
    std::vector<std::pair<Cube, glm::vec3>> cubes;
    std::vector<std::pair<TexturedCube, glm::vec3>> tex_cubes;
    std::vector<std::pair<Cube, glm::vec3>> pnt_lights;
    std::vector<std::pair<TexturedQuad, glm::vec3>> quads;

    u32 fbo = 0;
    u32 rbo = 0;

    u16 width = 1280;
    u16 height = 720;
    std::string name = "Rose";
    glm::vec2 last_xy = { 0.0f, 0.0f };
};

void resize_callback(GLFWwindow* window, int width, int height);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void mouse_callback(GLFWwindow* window, double xpos, double ypos);

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

void process_input(GLFWwindow* window, float delta_time);
} // namespace rose

#endif
