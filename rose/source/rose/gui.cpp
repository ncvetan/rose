#include <rose/camera.hpp>
#include <rose/gui.hpp>
#include <rose/window.hpp>

#include <imgui.h>
#include <imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>

#include <numbers>

namespace rose {
namespace gui {

// note: right now, the gui is a bit hacked together. it is primarily used for debugging and observing
// how changing certain parameters impact various graphics components. in the future this should
// be a cleaner interface with less bugs.

static f32 dir_angle = std::numbers::pi / 2;
static bool first_frame = true;
static bool vp_open = true;
static bool controls_open = true;

void imgui(WindowGLFW& state) {
    ImGuiIO& io = ImGui::GetIO();

    ImGui::Begin("controls", &controls_open, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("FPS: %f", io.Framerate);
    
    ImGui::Text("global controls");
    ImGui::Separator();
    ImGui::SliderFloat("exposure", &state.app_state.exposure, 0.1f, 5.0f);    
    ImGui::Checkbox("enable bloom", &state.app_state.bloom_on);

    ImGui::BeginDisabled(!state.app_state.bloom_on);
    ImGui::SliderInt("num passes", &state.app_state.n_bloom_passes, 1, 10);
    ImGui::EndDisabled();

    ImGui::SeparatorText("global light");
    if (ImGui::SliderAngle("angle", &dir_angle, 0.0f, 180.0f)) {
        state.app_state.dir_light.direction.y = -std::sin(dir_angle) * 1.0f;
        state.app_state.dir_light.direction.z = std::cos(dir_angle) * 1.0f;
        state.app_state.dir_light.direction = glm::normalize(state.app_state.dir_light.direction);
    }
    ImGui::ColorEdit3("color", glm::value_ptr(state.app_state.dir_light.color));
    
    ImGui::SeparatorText("point lights");
    bool light_changed = false;
    if (ImGui::TreeNode("lights")) {
        for (size_t idx = 0; idx < state.entities.size(); idx++) {
            
            if (!(state.entities.flags[idx] & EntityFlags::EMIT_LIGHT)) {
                continue;
            }
            
            if (ImGui::TreeNode((void*)(intptr_t)idx, "light %d", idx)) {
                light_changed = 
                    ImGui::SliderFloat3("position", glm::value_ptr(state.entities.positions[idx]), -30.0f, 30.0f) 
                    || ImGui::ColorEdit3("color", glm::value_ptr(state.entities.light_props[idx].color)) ||
                    ImGui::SliderFloat("linear", &state.entities.light_props[idx].linear, 0.1f, 10.0f) 
                    || ImGui::SliderFloat("quad", &state.entities.light_props[idx].quad, 0.1f, 10.0f) ||
                    ImGui::SliderFloat("intensity", &state.entities.light_props[idx].intensity, 1.0f, 10.0f);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    ImGui::End();

    // render our framebuffer to an imgui window
    ImGui::Begin("viewport", &vp_open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    
    state.window_data.vp_focused = ImGui::IsWindowFocused();
    state.window_data.vp_rect.x_min = ImGui::GetWindowPos().x;
    state.window_data.vp_rect.y_min = ImGui::GetWindowPos().y;
    state.window_data.vp_rect.x_max = ImGui::GetWindowPos().x + ImGui::GetWindowSize().x;
    state.window_data.vp_rect.y_max = ImGui::GetWindowPos().y + ImGui::GetWindowSize().y;

    // center the image in the viewport
    f32 scale = std::min(state.window_data.vp_rect.width() / state.window_data.width, state.window_data.vp_rect.height() / state.window_data.height);
    f32 offset_x = (state.window_data.vp_rect.width() - scale * (f32)state.window_data.width) / 2;
    f32 offset_y = (state.window_data.vp_rect.height() - scale * (f32)state.window_data.height) / 2;
    ImGui::SetCursorPos({ offset_x, offset_y });

    // resize the image based on the size of the viewport
    
    ImGui::Image(static_cast<ImTextureID>(state.fbuf_out.tex_bufs[0]), { scale * (f32)state.window_data.width, scale * (f32)state.window_data.height }, { 0, 1 }, { 1, 0 });
    
    ImGui::End();

    // setting an initial docking layout
    if (first_frame) {
        auto dock_id = ImGui::DockSpaceOverViewport();
        ImGui::DockBuilderAddNode(state.window_data.dock_id);
        ImGui::DockBuilderSetNodePos(state.window_data.dock_id, ImGui::GetWindowPos());
        ImGui::DockBuilderSetNodeSize(state.window_data.dock_id, ImGui::GetWindowSize());
        ImGuiID controls_node;
        ImGuiID viewport_node;
        ImGui::DockBuilderSplitNode(state.window_data.dock_id, ImGuiDir_Right, 0.85f, &viewport_node, &controls_node);
        ImGui::DockBuilderDockWindow("controls", controls_node);
        ImGui::DockBuilderDockWindow("viewport", viewport_node);
        first_frame = false;
    }

    if (light_changed) {
        state.entities.update_light_radii(state.app_state.exposure);
        update_light_state(state.entities, state.clusters);
    }
}

} // namespace gui
} // namespace rose