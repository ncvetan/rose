#ifndef ROSE_INCLUDE_WINDOW
#define ROSE_INCLUDE_WINDOW

#include <rose/camera.hpp>
#include <rose/err.hpp>
#include <rose/math.hpp>
#include <rose/model.hpp>
#include <rose/shader.hpp>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <optional>
#include <unordered_map>

namespace rose {

template <typename T>
concept drawable = requires(T d, ShaderGL& shader, const WorldState& state) {
    { d.draw(shader, state) } -> std::same_as<void>;
};

// model + additional context
template <drawable T>
struct Object {
    Object() = default;
    Object(const glm::vec3& pos) : world_pos(pos) {};
    Object(const glm::vec3& pos, const glm::vec3& scale) : world_pos(pos), scale(scale) {};

    T model;
    glm::vec3 world_pos = { 0.0f, 0.0f, 0.0f };
    glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
};

struct DirLight {
    glm::vec3 direction = { 0.0f, -1.0f, 0.0f };
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct PointLight {
    glm::vec3 ambient = { 0.02f, 0.02f, 0.02f };
    glm::vec3 diffuse = { 0.05f, 0.05f, 0.05f };
    glm::vec3 specular = { 0.2f, 0.2f, 0.2f };
    float attn_const = 1.0f;
    float attn_lin = 0.09f;
    float attn_quad = 0.032f;
};

// Object with additional light properties
template <drawable T>
struct LightObject {
    LightObject() = default;
    LightObject(const glm::vec3& pos) : object(pos) {};
    LightObject(const glm::vec3& pos, const glm::vec3& scale) : object(pos, scale) {};

    Object<T> object;
    PointLight light;
};

struct WorldState {
    SkyBox sky_box;
    DirLight dir_light;
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

    WorldState world_state;
    std::vector<Object<Model>> objects;
    std::vector<Object<TexturedCube>> tex_cubes;
    std::vector<LightObject<Cube>> pnt_lights;

    u32 fbo = 0;
    u32 rbo = 0;
    TextureRef fbo_tex;
    u16 width = 1920;
    u16 height = 1080;
    ImGuiID dock_id = 0;
    std::string name = "Rose";
    glm::vec2 last_xy;
    
    vec2f mouse_xy;
    bool vp_focused;
    Rectf vp_rect;
    bool glfw_captured = true;
};

void resize_callback(GLFWwindow* window, int width, int height);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void mouse_callback(GLFWwindow* window, double xpos, double ypos);

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

void process_input(GLFWwindow* window, float delta_time);

} // namespace rose

#endif
