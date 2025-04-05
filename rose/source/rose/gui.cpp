#include <rose/camera.hpp>
#include <rose/gui.hpp>
#include <rose/gl/gl_platform.hpp>

#include <imgui.h>
#include <imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>

#include <numbers>

#define NOMINMAX
#include <windows.h>

namespace rose {
namespace gui {

// note: right now, the gui is a bit hacked together. it is primarily used for debugging and observing
// how changing certain parameters impact various graphics components. in the future this should
// be a cleaner interface with less bugs.

// state used in the gui, global for now
namespace gui_state {
static f32 dir_angle = std::numbers::pi / 2;
static bool first_frame = true;
static bool vp_open = true;
static bool controls_open = true;

static char left_path[256] = "";
static char right_path[256] = "";
static char top_path[256] = "";
static char bottom_path[256] = "";
static char front_path[256] = "";
static char back_path[256] = "";

} // namespace gui_state

static fs::path open_windows_explorer() {
    OPENFILENAME ofn; 
    TCHAR szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);

    ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) == TRUE) {
        return fs::path(ofn.lpstrFile);
    }

    return "";
}

void gl_imgui(AppData& app_data, GL_Platform& platform) {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiID skybox_popup_id = ImHashStr("import_skybox_popup");

    // menu bar ===================================================================================

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Import Model")) {
                fs::path model_path = open_windows_explorer();
                EntityCtx ent_def = { 
                    .model_pth = model_path,
                    .pos = { 0.0f, 0.0f, 0.0f },
                    .scale = { 1.0f, 1.0f, 1.0f }, 
                    .light_props = PtLight(), 
                    .flags = EntityFlags::NONE 
                };
                platform.entities.add_object(platform.texture_manager, ent_def);
            }
            if (ImGui::MenuItem("Import SkyBox")) {
                ImGui::PushOverrideID(skybox_popup_id);
                ImGui::OpenPopup("import_skybox_popup");
                ImGui::PopID();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    ImGui::PushOverrideID(skybox_popup_id);
    if (ImGui::BeginPopupModal("import_skybox_popup")) {
        ImGui::InputText("left##skybox", gui_state::left_path, IM_ARRAYSIZE(gui_state::left_path));
        ImGui::SameLine();
        if (ImGui::Button("browse##1", ImVec2(50, 0))) {
            if (auto ret = open_windows_explorer(); ret != "") {
                std::strncat(gui_state::left_path, ret.string().c_str(), 255);
            }
        }
        ImGui::InputText("right##skybox", gui_state::right_path, IM_ARRAYSIZE(gui_state::right_path));
        ImGui::SameLine();
        if (ImGui::Button("browse##2", ImVec2(50, 0))) {
            if (auto ret = open_windows_explorer(); ret != "") {
                std::strncat(gui_state::right_path, ret.string().c_str(), 255);
            }
        }
        ImGui::InputText("top##skybox", gui_state::top_path, IM_ARRAYSIZE(gui_state::top_path));
        ImGui::SameLine();
        if (ImGui::Button("browse##3", ImVec2(50, 0))) {
            if (auto ret = open_windows_explorer(); ret != "") {
                std::strncat(gui_state::top_path, ret.string().c_str(), 255);
            }
        }
        ImGui::InputText("bottom##skybox", gui_state::bottom_path, IM_ARRAYSIZE(gui_state::bottom_path));
        ImGui::SameLine();
        if (ImGui::Button("browse##4", ImVec2(50, 0))) {
            if (auto ret = open_windows_explorer(); ret != "") {
                std::strncat(gui_state::bottom_path, ret.string().c_str(), 255);
            }
        }
        ImGui::InputText("front##skybox", gui_state::front_path, IM_ARRAYSIZE(gui_state::front_path));
        ImGui::SameLine();
        if (ImGui::Button("browse##5", ImVec2(50, 0))) {
            if (auto ret = open_windows_explorer(); ret != "") {
                std::strncat(gui_state::front_path, ret.string().c_str(), 255);
            }
        }
        ImGui::InputText("back##skybox", gui_state::back_path, IM_ARRAYSIZE(gui_state::back_path));
        ImGui::SameLine();
        if (ImGui::Button("browse##6", ImVec2(50, 0))) {
            if (auto ret = open_windows_explorer(); ret != "") {
                std::strncat(gui_state::back_path, ret.string().c_str(), 255);
            }
        }
        if (ImGui::Button("import##skybox_popup", ImVec2(50, 0))) {
            platform.platform_state.sky_box.load(platform.texture_manager, 
                                                 { gui_state::right_path, gui_state::left_path, gui_state::top_path,
                                                   gui_state::bottom_path, gui_state::front_path, gui_state::back_path });
        }
        if (ImGui::Button("close##skybox_popup", ImVec2(50, 0))) {
			ImGui::CloseCurrentPopup();
		}
        ImGui::EndPopup();
    }
    ImGui::PopID();

    // TODO: Not a huge fan of this but not a big deal right now
    bool light_changed = false;
    ImGui::Begin("controls", &gui_state::controls_open, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("FPS: %f (%f ms)", io.Framerate, 1000/ io.Framerate);

    ImGui::Text("global controls");
    ImGui::Separator();
    if (ImGui::SliderFloat("exposure", &app_data.window_data.exposure, 0.1f, 5.0f)) {
        light_changed = true;
    }
    ImGui::Checkbox("enable bloom", &app_data.bloom_on);

    ImGui::BeginDisabled(!app_data.bloom_on);
    ImGui::SliderInt("num passes", &app_data.n_bloom_passes, 1, 10);
    ImGui::EndDisabled();

    // directional light ==========================================================================

    ImGui::SeparatorText("global light");
    if (ImGui::SliderAngle("angle", &gui_state::dir_angle, 30.0f, 150.0f)) {
        platform.platform_state.dir_light.direction.y = -std::sin(gui_state::dir_angle) * 1.0f;
        platform.platform_state.dir_light.direction.z = std::cos(gui_state::dir_angle) * 1.0f;
        platform.platform_state.dir_light.direction = glm::normalize(platform.platform_state.dir_light.direction);
    }
    ImGui::ColorEdit3("color", glm::value_ptr(platform.platform_state.dir_light.color));
    
    // entities ===================================================================================

    ImGui::SeparatorText("entities");
    if (ImGui::TreeNode("entities")) {
        for (size_t idx = 0; idx < platform.entities.size(); idx++) {
            if (ImGui::TreeNode((void*)(intptr_t)idx, "ent %d", idx)) {
                if (is_set(platform.entities.flags[idx], EntityFlags::EMIT_LIGHT))
                {
                    light_changed = light_changed || ImGui::SliderFloat3("position", glm::value_ptr(platform.entities.positions[idx]), -30.0f, 30.0f);
                    if (ImGui::Button("toggle light")) {
                        platform.entities.flags[idx] &= ~EntityFlags::EMIT_LIGHT;
                        light_changed = light_changed || true;
                    } 
                    light_changed = light_changed || 
                                     ImGui::ColorEdit3("color", glm::value_ptr(platform.entities.light_props[idx].color)) ||
                                     ImGui::SliderFloat("linear", &platform.entities.light_props[idx].linear, 0.1f, 10.0f) || 
                                     ImGui::SliderFloat("quad", &platform.entities.light_props[idx].quad, 0.1f, 10.0f) ||
                                     ImGui::SliderFloat("intensity", &platform.entities.light_props[idx].intensity, 1.0f, 10.0f);
                    ImGui::TreePop();
                } 
                else {
                    ImGui::SliderFloat3("position", glm::value_ptr(platform.entities.positions[idx]), -30.0f, 30.0f);
                    if (ImGui::Button("toggle light")) {
                        platform.entities.flags[idx] |= EntityFlags::EMIT_LIGHT;
                        light_changed = light_changed || true;
                    }
                    ImGui::BeginDisabled(true);
                    ImGui::ColorEdit3("color", glm::value_ptr(platform.entities.light_props[idx].color)) ||
                    ImGui::SliderFloat("linear", &platform.entities.light_props[idx].linear, 0.1f, 10.0f) ||
                    ImGui::SliderFloat("quad", &platform.entities.light_props[idx].quad, 0.1f, 10.0f) ||
                    ImGui::SliderFloat("intensity", &platform.entities.light_props[idx].intensity, 1.0f, 10.0f);
                    ImGui::EndDisabled();
                    ImGui::TreePop();
                }
            }
        }
        ImGui::TreePop();
    }
    ImGui::End();

    // render our framebuffer to an imgui window
    ImGui::Begin("viewport", &gui_state::vp_open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    
    app_data.window_data.vp_focused = ImGui::IsWindowFocused();
    app_data.window_data.vp_rect.x_min = ImGui::GetWindowPos().x;
    app_data.window_data.vp_rect.y_min = ImGui::GetWindowPos().y;
    app_data.window_data.vp_rect.x_max = ImGui::GetWindowPos().x + ImGui::GetWindowSize().x;
    app_data.window_data.vp_rect.y_max = ImGui::GetWindowPos().y + ImGui::GetWindowSize().y;

    // center the image in the viewport
    f32 scale = std::min(app_data.window_data.vp_rect.width() / app_data.window_data.width, app_data.window_data.vp_rect.height() / app_data.window_data.height);
    f32 offset_x = (app_data.window_data.vp_rect.width() - scale * (f32)app_data.window_data.width) / 2;
    f32 offset_y = (app_data.window_data.vp_rect.height() - scale * (f32)app_data.window_data.height) / 2;
    ImGui::SetCursorPos({ offset_x, offset_y });

    // resize the image based on the size of the viewport
    
    ImGui::Image(static_cast<ImTextureID>(platform.fbuf_out.tex_bufs[0]), 
        { scale * (f32)app_data.window_data.width, scale * (f32)app_data.window_data.height }, { 0, 1 }, { 1, 0 });
    
    ImGui::End();

    // setting an initial docking layout
    if (gui_state::first_frame) {
        ImGui::DockBuilderAddNode(app_data.window_data.dock_id);
        ImGui::DockBuilderSetNodePos(app_data.window_data.dock_id, ImGui::GetWindowPos());
        ImGui::DockBuilderSetNodeSize(app_data.window_data.dock_id, ImGui::GetWindowSize());
        ImGuiID controls_node;
        ImGuiID viewport_node;
        ImGui::DockBuilderSplitNode(app_data.window_data.dock_id, ImGuiDir_Right, 0.85f, &viewport_node, &controls_node);
        ImGui::DockBuilderDockWindow("controls", controls_node);
        ImGui::DockBuilderDockWindow("viewport", viewport_node);
        gui_state::first_frame = false;
    }

    // update entity state
    // TODO: Not a fan on this, but haven't come up with a better solution yet
    if (light_changed) {
        platform.entities.update_light_radii();
        update_light_ssbos(platform.entities, platform.clusters);
    }
}

} // namespace gui
} // namespace rose