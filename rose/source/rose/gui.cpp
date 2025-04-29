#include <rose/camera.hpp>
#include <rose/gui.hpp>
#include <rose/gl/platform.hpp>

#include <imgui.h>
#include <imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>

#include <numbers>

#define NOMINMAX
#include <windows.h>

namespace gui {

// note: right now, the gui is a bit hacked together. it is primarily used for debugging and observing
// how changing certain parameters impact various graphics components.

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

// note: ideally, this shouldn't be coupled with the graphics API, but I haven't created a clean delineation between
// systems that are dependant/non-dependant on API, and therefore can not decouple it yet
GuiRet gl_imgui(AppState& app_state, gl::Platform& platform) {
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
                    .rotation = { 0.0f, 0.0f, 0.0f },
                    .light_data = PtLightData(), 
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

    // global controls ============================================================================

    bool light_changed = false; // TODO: Not a huge fan of this but not a big deal right now
    ImGui::Begin("controls", &gui_state::controls_open, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("FPS: %.2f (%.2f ms)", io.Framerate, 1000.0f / io.Framerate);

    ImGui::Text("global controls");
    ImGui::Separator();
    ImGui::SliderFloat("exposure", &app_state.window_state.exposure, 0.01f, 2.5f);
    if (ImGui::Checkbox("toggle vsync", &app_state.window_state.vsync_enabled)) {
        (app_state.window_state.vsync_enabled) ? glfwSwapInterval(1) : glfwSwapInterval(0);
    }
    ImGui::Checkbox("toggle bloom", &app_state.bloom_enabled);
    ImGui::BeginDisabled(!app_state.bloom_enabled);
    if (ImGui::SliderInt("num passes", &app_state.n_bloom_passes, 1, 10));
    if (ImGui::SliderFloat("threshold", &app_state.bloom_threshold, 0.1f, 5.0f)) {
        platform.shaders.brightness.set_f32("bloom_threshold", app_state.bloom_threshold);
        platform.shaders.bloom.set_f32("bloom_threshold", app_state.bloom_threshold);
    }
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

            if (!platform.entities.is_alive(idx)) {
                continue;
            }
            
            if (ImGui::TreeNode((void*)(intptr_t)idx, "ent %d", platform.entities.ids[idx])) {
                
                if (ImGui::Button("+")) {  // duplicate
                    if (platform.entities.is_light(idx)) {
                        light_changed = true;
                    }
                    platform.entities.dup_object(idx);
                }
                ImGui::SameLine();
                if (ImGui::Button("-")) {  // delete
                    if (platform.entities.is_light(idx)) {
                        light_changed = true;
                    }
                    platform.entities.del_object(idx);
                }
                
                if (ImGui::SliderFloat3("position", glm::value_ptr(platform.entities.positions[idx]), -30.0f, 30.0f)) {
                    if (platform.entities.is_light(idx)) {
                        light_changed = true;
                    }
                }
                if (ImGui::SliderFloat3("rotation", glm::value_ptr(platform.entities.rotations[idx]), 0.0f, 360.0f)) {
                    if (platform.entities.is_light(idx)) {
                        light_changed = true;
                    }
                }
                if (ImGui::SliderFloat("scale", &platform.entities.scales[idx].x, 0.1f, 10.0f)) {
                    // note: using a single float slider to set all values in a vec3
                    platform.entities.scales[idx] = { platform.entities.scales[idx].x,
                                                      platform.entities.scales[idx].x,
                                                      platform.entities.scales[idx].x };
                }

                if (ImGui::Button("toggle light")) {
                    if (platform.entities.is_light(idx)) {
                        set_flag(platform.entities.flags[idx], EntityFlags::EMIT_LIGHT);
                    } 
                    else {
                        unset_flag(platform.entities.flags[idx], EntityFlags::EMIT_LIGHT);
                    }
                    light_changed = true;
                } 

                ImGui::BeginDisabled(!platform.entities.is_light(idx));
                if (ImGui::Button("cast shadows")) {
                    platform.entities.pt_caster_idx = idx;
                    platform.shaders.lighting_deferred.set_u32("pt_caster_id", platform.entities.ids[platform.entities.pt_caster_idx]);
                    platform.shaders.lighting_forward.set_u32("pt_caster_id", platform.entities.ids[platform.entities.pt_caster_idx]);
                }
                if (ImGui::ColorEdit3("color", &platform.entities.light_data[idx].color.x)) {
                    light_changed = true;
                }
                if (ImGui::SliderFloat("radius", &platform.entities.light_data[idx].radius, 0.1, 100.0f)) {
                    light_changed = true;
                }
                if (ImGui::SliderFloat("intensity", &platform.entities.light_data[idx].intensity, 1.0f, 10.0f)) {
                    light_changed = true;
                }
                ImGui::EndDisabled();
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    ImGui::End();

    // render our framebuffer to an imgui window
    ImGui::Begin("viewport", &gui_state::vp_open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    
    app_state.window_state.vp_focused = ImGui::IsWindowFocused();
    app_state.window_state.vp_rect.x_min = ImGui::GetWindowPos().x;
    app_state.window_state.vp_rect.y_min = ImGui::GetWindowPos().y;
    app_state.window_state.vp_rect.x_max = ImGui::GetWindowPos().x + ImGui::GetWindowSize().x;
    app_state.window_state.vp_rect.y_max = ImGui::GetWindowPos().y + ImGui::GetWindowSize().y;

    // center the image in the viewport
    f32 scale = std::min(app_state.window_state.vp_rect.width() / app_state.window_state.width, app_state.window_state.vp_rect.height() / app_state.window_state.height);
    f32 offset_x = (app_state.window_state.vp_rect.width() - scale * (f32)app_state.window_state.width) / 2;
    f32 offset_y = (app_state.window_state.vp_rect.height() - scale * (f32)app_state.window_state.height) / 2;
    ImGui::SetCursorPos({ offset_x, offset_y });

    // resize the image based on the size of the viewport
    
    ImGui::Image(static_cast<ImTextureID>(platform.out_fbuf.tex_bufs[0]), 
        { scale * (f32)app_state.window_state.width, scale * (f32)app_state.window_state.height }, { 0, 1 }, { 1, 0 });
    
    ImGui::End();

    // setting an initial docking layout
    if (gui_state::first_frame) {
        if (!std::filesystem::exists("imgui.ini")) {
            ImGui::DockBuilderAddNode(app_state.window_state.dock_id);
            ImGui::DockBuilderSetNodePos(app_state.window_state.dock_id, ImGui::GetWindowPos());
            ImGui::DockBuilderSetNodeSize(app_state.window_state.dock_id, ImGui::GetWindowSize());
            ImGuiID controls_node;
            ImGuiID viewport_node;
            ImGui::DockBuilderSplitNode(app_state.window_state.dock_id, ImGuiDir_Right, 0.85f, &viewport_node, &controls_node);
            ImGui::DockBuilderDockWindow("controls", controls_node);
            ImGui::DockBuilderDockWindow("viewport", viewport_node);
        }
        gui_state::first_frame = false;
    }

    return { .light_changed = light_changed };
}

} // namespace gui