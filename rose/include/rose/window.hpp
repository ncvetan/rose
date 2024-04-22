#ifndef ROSE_INCLUDE_WINDOW
#define ROSE_INCLUDE_WINDOW

#include <rose/camera.hpp>
#include <rose/err.hpp>
#include <rose/model.hpp>
#include <rose/shader.hpp>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <optional>

namespace rose {

template <typename T>
concept drawable = requires(T d, ShaderGL& shader, const WorldState& state) {
    { d.draw(shader, state) } -> std::same_as<void>;
};

template <drawable T>
struct Object {

    Object() = default;
    Object(const glm::vec3& pos) : world_pos(pos){};
    Object(const glm::vec3& pos, const glm::vec3& scale) : world_pos(pos), scale(scale) {};

    T object;
    glm::vec3 world_pos;
    glm::vec3 scale = glm::vec3(1.0);
};

struct WorldState {
    SkyBox sky_box;
    uint32_t ubo;
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

    std::unordered_map<std::string, ShaderGL> shaders;

    WorldState state;

    std::vector<Object<Model>> objects;
    std::vector<Object<TexturedCube>> tex_cubes;

    u32 fbo = 0;
    u32 rbo = 0;
    TexturedQuad fbo_quad;

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
