#include <rose/camera.hpp>
#include <rose/gui.hpp>
#include <rose/window.hpp>

#include <imgui.h>
#include <imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>s

#include <numbers>

namespace rose {
namespace gui {

static float dir_angle = std::numbers::pi / 2;
static bool first_frame = true;
static bool vp_open = true;

void imgui(WindowGLFW& state) {

    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::Begin("controls", (bool*)true, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("FPS: %f", io.Framerate);
    
    ImGui::Text("light controls");
    ImGui::Separator();
    ImGui::SeparatorText("global light");
    if (ImGui::SliderAngle("angle", &dir_angle, 0.0f, 180.0f)) {
        state.world_state.dir_light.direction.y = -std::sin(dir_angle) * 1.0f;
        state.world_state.dir_light.direction.z = std::cos(dir_angle) * 1.0f;
    }
    ImGui::ColorEdit3("ambient", glm::value_ptr(state.world_state.dir_light.ambient));
    ImGui::ColorEdit3("diffuse", glm::value_ptr(state.world_state.dir_light.diffuse));
    ImGui::ColorEdit3("specular", glm::value_ptr(state.world_state.dir_light.specular));
    
    ImGui::SeparatorText("point lights");
    if (ImGui::TreeNode("lights")) {
        for (int i = 0; i < state.pnt_lights.size(); i++) {
            if (ImGui::TreeNode((void*)(intptr_t)i, "light %d", i)) {
                ImGui::SliderFloat3("position", glm::value_ptr(state.pnt_lights[i].object.world_pos), -10.0f, 10.0f);
                ImGui::ColorEdit3("ambient", glm::value_ptr(state.pnt_lights[i].light.ambient));
                ImGui::ColorEdit3("diffuse", glm::value_ptr(state.pnt_lights[i].light.diffuse));
                ImGui::ColorEdit3("specular", glm::value_ptr(state.pnt_lights[i].light.specular));
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }

    ImGui::End();

    // render our framebuffer to an imgui window
    ImGui::Begin("viewport", &vp_open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    state.vp_focused = ImGui::IsWindowFocused();
    state.vp_rect.x_min = ImGui::GetWindowPos().x;
    state.vp_rect.y_min = ImGui::GetWindowPos().y;
    state.vp_rect.x_max = ImGui::GetWindowPos().x + ImGui::GetWindowSize().x;
    state.vp_rect.y_max = ImGui::GetWindowPos().y + ImGui::GetWindowSize().y;
    ImGui::Image((void*)state.fbo_tex->id, { (float)state.width, (float)state.height }, { 0, 1 }, { 1, 0 });
    ImGui::End();

    // setting an initial docking layout
    if (first_frame) {
        ImGui::DockBuilderAddNode(state.dock_id);
        ImGui::DockBuilderSetNodePos(state.dock_id, ImGui::GetWindowPos());
        ImGui::DockBuilderSetNodeSize(state.dock_id, ImGui::GetWindowSize());
        ImGuiID controls_node;
        ImGuiID viewport_node;
        ImGui::DockBuilderSplitNode(state.dock_id, ImGuiDir_Right, 0.85f, &viewport_node, &controls_node);
        ImGui::DockBuilderDockWindow("controls", controls_node);
        ImGui::DockBuilderDockWindow("viewport", viewport_node);
        first_frame = false;
    }
}

} // namespace gui
} // namespace rose