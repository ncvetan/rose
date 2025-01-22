#ifndef ROSE_INCLUDE_WINDOW
#define ROSE_INCLUDE_WINDOW

#include <rose/camera.hpp>
#include <rose/err.hpp>
#include <rose/glstructs.hpp>
#include <rose/math.hpp>
#include <rose/model.hpp>
#include <rose/shader.hpp>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <optional>
#include <unordered_map>

namespace rose {

// TODO: This entire thing is much more than a 'window' at this point and needs to be refactored
class WindowGLFW {
  public:
    WindowGLFW() = default;

    [[nodiscard]] std::optional<rses> init();
    void enable_vsync(bool enable);
    void update();
    void destroy();

    GLFWwindow* window = nullptr;
    CameraGL camera;
    ShadersGL shaders;
    TextureManager texture_manager;

    GlobalState world_state;
    Objects objects;

    FrameBuf gbuf;
    FrameBuf pp1;
    FrameBuf pp2;
    FrameBuf fbuf_out;

    u32 width = 1920;
    u32 height = 1080;
    ImGuiID dock_id = 0;
    std::string name = "Rose";
    glm::vec2 last_xy;

    vec2f mouse_xy;
    Rectf vp_rect;
    bool vp_focused = false;
    bool glfw_captured = true;

    // TODO: move this somewhere else
    void update_light_ssbos() {
        std::vector<PointLight> pnt_lights_props;
        std::vector<glm::vec4> pnt_lights_pos;
        u32 n_lights = 0;

        // linear search, probably not a big deal
        for (size_t idx = 0; idx < objects.size(); ++idx) {
            if (objects.flags[idx] & ObjectFlags::EMIT_LIGHT) {
                pnt_lights_pos.push_back(glm::vec4(objects.posns[idx], 1.0f));
                pnt_lights_props.push_back(objects.light_props[idx]);
                n_lights++;
            }
        }
        
        clusters.lights_ssbo.update(0, std::span(pnt_lights_props.begin(), pnt_lights_props.end()));
        clusters.lights_pos_ssbo.update(0, std::span(pnt_lights_pos.begin(), pnt_lights_pos.end()));

        // zero out the rest of the buffer
        clusters.lights_ssbo.zero(n_lights);
        clusters.lights_pos_ssbo.zero(n_lights);
    }

    ClusterCtx clusters;

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
