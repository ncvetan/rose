#ifndef ROSE_INCLUDE_WINDOW
#define ROSE_INCLUDE_WINDOW

#include <rose/camera.hpp>
#include <rose/err.hpp>
#include <rose/math.hpp>
#include <rose/model.hpp>
#include <rose/object.hpp>
#include <rose/shader.hpp>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <optional>
#include <unordered_map>

namespace rose {

struct FrameBufTexCtx {
    GLenum intern_format = 0;
    GLenum format = 0;
    GLenum type = 0;
};

struct FrameBuf {
    
    struct Vertex {
        glm::vec3 pos;
        glm::vec2 tex;
    };
    
    ~FrameBuf();

    [[nodiscard]] std::optional<rses> init(int w, int h, const std::vector<FrameBufTexCtx>& texs);
    void draw(ShaderGL& shader, const GlobalState& state);

    u32 frame_buf = 0;
    u32 render_buf = 0;
    std::vector<u32> tex_bufs;
    std::vector<GLenum> attachments;

    u32 vertex_arr = 0;
    u32 vertex_buf = 0;

    std::vector<Vertex> verts = {
        { { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },   { { -1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
        { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } }, { { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
        { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } }, { { 1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } }
    };

};

class WindowGLFW {
  public:
    WindowGLFW() = default;

    [[nodiscard]] std::optional<rses> init();
    void enable_vsync(bool enable);
    void update();
    void destroy();

    GLFWwindow* window = nullptr;

    CameraGL camera;
    TextureManager texture_manager;
    std::unordered_map<std::string, ShaderGL> shaders;

    GlobalState world_state;
    std::vector<Object<Model>> objects;
    std::vector<Object<TexturedCube>> tex_cubes;
    std::vector<Object<Cube>> pnt_lights;

    FrameBuf gbuf;
    FrameBuf pp1;
    FrameBuf pp2;
    FrameBuf fbuf_out;

    u16 width = 1920;
    u16 height = 1080;
    ImGuiID dock_id = 0;
    std::string name = "Rose";
    glm::vec2 last_xy;

    vec2f mouse_xy;
    Rectf vp_rect;
    bool vp_focused = false;
    bool glfw_captured = true;

  private:
    std::optional<rses> init_glfw();
    std::optional<rses> init_imgui(GLFWwindow* window);
    std::optional<rses> init_opengl();
};

void resize_callback(GLFWwindow* window, int width, int height);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void mouse_callback(GLFWwindow* window, double xpos, double ypos);

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

void process_input(GLFWwindow* window, float delta_time);

} // namespace rose

#endif
