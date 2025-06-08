#include <rose/camera.hpp>
#include <rose/gui.hpp>

#ifdef USE_OPENGL
#include <rose/backends/gl/backend.hpp>
#else
static_assert("no backend selected");
#endif

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

std::vector<i64> ent_traverse = { 0, 1 }; // entity indices in order of object insertion

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

// TODO: ideally, this shouldn't be coupled with the graphics API, but I haven't created a clean delineation between
// systems that are dependant/non-dependant on API, and therefore can not decouple it yet
GuiRet imgui(AppState& app_state, gl::Backend& backend) {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiID skybox_popup_id = ImHashStr("import_skybox_popup");

    // menu bar ===================================================================================

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Import Model")) {
                fs::path model_path = open_windows_explorer();
                EntityCtx ent_def = { 
                    .model_path = model_path,
                    .pos = { 0.0f, 0.0f, 0.0f },
                    .scale = { 1.0f, 1.0f, 1.0f }, 
                    .rotation = { 0.0f, 0.0f, 0.0f },
                    .light_data = PtLight(), 
                    .flags = EntityFlags::NONE 
                };
                gui_state::ent_traverse.push_back(app_state.entities.add_object(backend.texture_manager, ent_def));
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
            backend.backend_state.skybox.load(backend.texture_manager, 
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
    bool obj_deleted = false;
    std::vector<i64>::iterator del_iter;

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
    ImGui::SliderFloat("bloom factor", &app_state.bloom_factor, 0.005f, 0.25f);
    ImGui::EndDisabled();

    // directional light ==========================================================================

    ImGui::SeparatorText("global light");
    if (ImGui::SliderAngle("angle", &gui_state::dir_angle, 30.0f, 150.0f)) {
        backend.backend_state.dir_light.direction.y = -std::sin(gui_state::dir_angle) * 1.0f;
        backend.backend_state.dir_light.direction.z = std::cos(gui_state::dir_angle) * 1.0f;
        backend.backend_state.dir_light.direction = glm::normalize(backend.backend_state.dir_light.direction);
    }
    ImGui::ColorEdit3("color", glm::value_ptr(backend.backend_state.dir_light.color));
    
    // entities ===================================================================================

    ImGui::SeparatorText("entities");
    if (ImGui::TreeNode("entities")) {
        for (i64 traverse_idx = 0; traverse_idx < gui_state::ent_traverse.size(); ++traverse_idx) {

            i64 ent_idx = gui_state::ent_traverse[traverse_idx];

            if (!app_state.entities.is_alive(ent_idx)) {
                continue;
            }
            
            if (ImGui::TreeNode((void*)(intptr_t)ent_idx, "ent %d", app_state.entities.ids[ent_idx])) {
                
                if (ImGui::Button("+")) {  // duplicate
                    if (app_state.entities.is_light(ent_idx)) {
                        light_changed = true;
                    }
                    gui_state::ent_traverse.push_back(app_state.entities.dup_object(ent_idx));
                }
                ImGui::SameLine();
                if (ImGui::Button("-")) {  // delete
                    if (app_state.entities.is_light(ent_idx)) {
                        light_changed = true;
                    }

                    obj_deleted = true;
                    del_iter = gui_state::ent_traverse.begin() + traverse_idx;
                    app_state.entities.del_object(ent_idx);
                }
                
                if (ImGui::SliderFloat3("position", glm::value_ptr(app_state.entities.positions[ent_idx]), -30.0f, 30.0f)) {
                    if (app_state.entities.is_light(ent_idx)) {
                        light_changed = true;
                    }
                }
                if (ImGui::SliderFloat3("rotation", glm::value_ptr(app_state.entities.rotations[ent_idx]), 0.0f, 360.0f)) {
                    if (app_state.entities.is_light(ent_idx)) {
                        light_changed = true;
                    }
                }
                if (ImGui::SliderFloat("scale", &app_state.entities.scales[ent_idx].x, 0.025f, 10.0f)) {
                    // note: using a single float slider to set all values in a vec3
                    app_state.entities.scales[ent_idx] = { app_state.entities.scales[ent_idx].x,
                                                      app_state.entities.scales[ent_idx].x,
                                                      app_state.entities.scales[ent_idx].x };
                }

                if (ImGui::Button("toggle light")) {
                    if (app_state.entities.is_light(ent_idx)) {
                        set_flag(app_state.entities.flags[ent_idx], EntityFlags::EMIT_LIGHT);
                    } 
                    else {
                        unset_flag(app_state.entities.flags[ent_idx], EntityFlags::EMIT_LIGHT);
                    }
                    light_changed = true;
                } 

                ImGui::BeginDisabled(!app_state.entities.is_light(ent_idx));
                if (ImGui::Button("cast shadows")) {
                    app_state.entities.pt_caster_idx = ent_idx;
                    backend.shaders.lighting_deferred.set_u32("pt_caster_id", app_state.entities.ids[app_state.entities.pt_caster_idx]);
                    backend.shaders.lighting_forward.set_u32("pt_caster_id", app_state.entities.ids[app_state.entities.pt_caster_idx]);
                }
                if (ImGui::ColorEdit3("color", &app_state.entities.light_data[ent_idx].color.x)) {
                    light_changed = true;
                }
                if (ImGui::SliderFloat("radius", &app_state.entities.light_data[ent_idx].radius, 0.1, 100.0f)) {
                    light_changed = true;
                }
                if (ImGui::SliderFloat("intensity", &app_state.entities.light_data[ent_idx].intensity, 1.0f, 10.0f)) {
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
    
    ImGui::Image(static_cast<ImTextureID>(backend.out_fbuf.tex_bufs[0]), 
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

    if (obj_deleted) {
        gui_state::ent_traverse.erase(del_iter);
    }

    return { .light_changed = light_changed };
}

} // namespace gui