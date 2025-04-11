#include <rose/gui.hpp>
#include <rose/model.hpp>
#include <rose/core/err.hpp>
#include <rose/gl/gl_init.hpp>
#include <rose/gl/gl_platform.hpp>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <stb_image.h>

#include <array>
#include <format>
#include <iostream>
#include <print>

namespace rose {

std::optional<rses> GL_Platform::init(AppState& app_state) {

    std::optional<rses> err = std::nullopt;

    if (err = init_opengl()) {
        return err.value().general("Unable to initialize GLEW and OpenGL");
    }

    if (err = shaders.init()) {
        return err.value().general("unable to initialize shaders");
    }

    // object initialization ======================================================================
    
    // note: these models are manually loaded here for my ease of use. if someone wanted to run the executable on their own,
    // they need to modify these paths for their own system. eventually, I'd like a format for savings scenes as well as
    // the ability to load models through the gui

    platform_state.sky_box.init();
    if (err = platform_state.sky_box.load(
            texture_manager, { SOURCE_DIR "/assets/skybox/right.jpg", SOURCE_DIR "/assets/skybox/left.jpg",
                               SOURCE_DIR "/assets/skybox/top.jpg", SOURCE_DIR "/assets/skybox/bottom.jpg",
                               SOURCE_DIR "/assets/skybox/front.jpg", SOURCE_DIR "/assets/skybox/back.jpg" })) {
        return err;
    }

    // note: hard coding some model loading for testing, can be removed eventually
    EntityCtx sponza_def = { .model_pth = SOURCE_DIR "/assets/Sponza/glTF/Sponza.gltf",
                             .pos = { 0.0f, 0.0f, 0.0f },
                             .scale = { 0.02f, 0.02f, 0.02f },
                             .light_props = PtLight(),
                             .flags = EntityFlags::NONE };

    entities.add_object(texture_manager, sponza_def);
    
    EntityCtx model2_def = {
                             .model_pth = SOURCE_DIR "/assets/sphere/scene.gltf",
                             .pos = { 0.0f, 3.5f, 0.0f },
                             .scale = { 0.1f, 0.1f, 0.1f },
                             .light_props = PtLight(),
                             .flags = EntityFlags::EMIT_LIGHT };

    entities.add_object(texture_manager, model2_def);
    entities.light_props.back().color = { 0.35f, 0.1f, 0.1f, 1.0f };

    // frame buf initialization ===================================================================
    
    // TODO: optimize size of framebuffers

    // (position, normal, albedo)
    if (err = gbuf_fbuf.init(app_state.window_state.width, app_state.window_state.height, true, { { GL_RGBA16F }, { GL_RGBA16F }, { GL_RGBA8 } })) {
        return err;
    }

    if (err = int_fbuf.init(app_state.window_state.width, app_state.window_state.height, false, { { GL_RGBA16F } })) {
        return err;
    }

    // note: pp1 uses the render buffer of gbuf to perform masking with the stencil buffer
    glNamedFramebufferRenderbuffer(int_fbuf.frame_buf, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gbuf_fbuf.render_buf);

    if (err = out_fbuf.init(app_state.window_state.width, app_state.window_state.height, false, { { GL_RGBA8 } })) {
        return err;
    }

    // uniform buffer initialization ==============================================================

    glCreateBuffers(1, &platform_state.global_ubo);
    glNamedBufferStorage(platform_state.global_ubo, 208, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, platform_state.global_ubo);

    glm::uvec2 screen_dims = { app_state.window_state.width, app_state.window_state.height };
    glNamedBufferSubData(platform_state.global_ubo, 176, 16, glm::value_ptr(clusters.grid_sz));
    glNamedBufferSubData(platform_state.global_ubo, 192, 8, glm::value_ptr(screen_dims));
    glNamedBufferSubData(platform_state.global_ubo, 200, 4, &app_state.camera.far_plane);
    glNamedBufferSubData(platform_state.global_ubo, 204, 4, &app_state.camera.near_plane);

    // ssbo initialization ========================================================================

    i32 n_clusters = clusters.grid_sz.x * clusters.grid_sz.y * clusters.grid_sz.z;
    clusters.clusters_aabb_ssbo.init(sizeof(AABB) * n_clusters, 2);
    clusters.lights_ssbo.init(sizeof(PtLight) * 1024, 3);
    clusters.lights_pos_ssbo.init(sizeof(glm::vec4) * 1024, 4);
    update_light_ssbos(entities, clusters);
    clusters.clusters_ssbo.init(sizeof(u32) * (1 + clusters.max_lights_in_cluster) * n_clusters, 5);

    // shadow map initialization ==================================================================
    
    // ---- directional shadow map ----

    if (err = init_dir_shadow(platform_state.dir_light.shadow_data)) {
        return err;
    }

    // ---- point shadow map ----

    if (err = init_pt_shadow(platform_state.pt_shadow_data)) {
        return err;
    }

    // remaining set up ===========================================================================

    shaders.brightness.set_f32("bloom_threshold", app_state.bloom_threshold);
    shaders.bloom.set_f32("bloom_threshold", app_state.bloom_threshold);

    return std::nullopt;
};

void GL_Platform::new_frame(AppState& app_state) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    app_state.window_state.dock_id = ImGui::DockSpaceOverViewport();
}

void GL_Platform::end_frame(GLFWwindow* window_handle) {
    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}

// obtains a projection-view matrix for a directional light
// near and far values should be specific to the cascade
static glm::mat4 get_light_pv(const Camera& camera, const glm::vec3& light_dir, const GL_PlatformState& state, f32 ar, f32 near, f32 far)
{
    std::array<glm::vec4, 8> frustum_corners = {
        glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f),
        glm::vec4( 1.0f, -1.0f, -1.0f, 1.0f),
        glm::vec4(-1.0f,  1.0f, -1.0f, 1.0f),
        glm::vec4( 1.0f,  1.0f, -1.0f, 1.0f),
        glm::vec4(-1.0f, -1.0f,  1.0f, 1.0f),
        glm::vec4( 1.0f, -1.0f,  1.0f, 1.0f),
        glm::vec4(-1.0f,  1.0f,  1.0f, 1.0f),
        glm::vec4( 1.0f,  1.0f,  1.0f, 1.0f)
    };

    glm::mat4 proj = glm::perspective(glm::radians(camera.zoom), ar, near, far);
    glm::mat4 pv_inv = glm::inverse(proj * camera.view());
    glm::vec4 frust_center;

    // [ clip -> world ]
    for (auto& corner : frustum_corners) {        
        corner = pv_inv * corner;
        corner /= corner.w;
        frust_center += corner;
    }

    frust_center /= 8.0f;

    f32 frust_radius = glm::length(frustum_corners[0] - frustum_corners[7]) / 2.0f;
    f32 tex_per_unit = state.dir_light.shadow_data.resolution / (frust_radius * 2.0f);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), { tex_per_unit, tex_per_unit, tex_per_unit });
    glm::mat4 look_at = scale * glm::lookAt({ 0.0f, 0.0f, 0.0f }, -light_dir, { 0.0f, 1.0f, 0.0f });
    glm::mat4 look_at_inv = glm::inverse(look_at);

    frust_center = frust_center * look_at;
    frust_center.x = std::floor(frust_center.x);
    frust_center.y = std::floor(frust_center.y);
    frust_center = frust_center * look_at_inv;

    glm::mat4 light_view = glm::lookAt(glm::vec3(frust_center) - (light_dir * frust_radius * 2.0f),
                                       glm::vec3(frust_center), { 0.0f, 1.0f, 0.0f });

    glm::mat4 light_proj = glm::ortho(-frust_radius, frust_radius, -frust_radius, frust_radius, -frust_radius * 5.0f, frust_radius * 5.0f);
    return light_proj * light_view;
}

void GL_Platform::render(AppState& app_state) {
   
    // frame set up ===============================================================================================
            
    glEnable(GL_DEPTH_TEST);

    f32 ar = (f32)app_state.window_state.width / (f32)app_state.window_state.height;
    glm::mat4 projection = app_state.camera.projection(ar);
    glm::mat4 view = app_state.camera.view();

    // update ubo state
    glNamedBufferSubData(platform_state.global_ubo, 0, 64, glm::value_ptr(projection));
    glNamedBufferSubData(platform_state.global_ubo, 64, 64, glm::value_ptr(view));
    glNamedBufferSubData(platform_state.global_ubo, 128, 16, glm::value_ptr(app_state.camera.position));
    glNamedBufferSubData(platform_state.global_ubo, 144, 16, glm::value_ptr(platform_state.dir_light.direction));
    glNamedBufferSubData(platform_state.global_ubo, 160, 16, glm::value_ptr(platform_state.dir_light.color));

    // clustered set-up ===========================================================================================

    // determine the AABB for each cluster
    // note: this only needs to be called once if parameters do not change
    shaders.clusters_build.use();
    shaders.clusters_build.set_mat4("inv_proj", glm::inverse(projection));

    glDispatchCompute(clusters.grid_sz.x, clusters.grid_sz.y, clusters.grid_sz.z);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // build light lists for each cluster
    shaders.clusters_cull.use();
    shaders.clusters_cull.set_i32("n_lights", clusters.lights_ssbo.n_elems);

    glDispatchCompute(27, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // shadow pass ================================================================================================
        
    glCullFace(GL_FRONT);  // prevent peter panning

    // ---- directional light ----

    glBindFramebuffer(GL_FRAMEBUFFER, platform_state.dir_light.shadow_data.fbo);
    glViewport(0, 0, platform_state.dir_light.shadow_data.resolution, platform_state.dir_light.shadow_data.resolution);
    glClear(GL_DEPTH_BUFFER_BIT);

    f32 c1_far = 10.0f;
    f32 c2_far = 30.0f;

    glm::mat4 c1_map = get_light_pv(app_state.camera, platform_state.dir_light.direction, platform_state, ar, app_state.camera.near_plane, c1_far);
    glm::mat4 c2_map = get_light_pv(app_state.camera, platform_state.dir_light.direction, platform_state, ar, c1_far, c2_far);
    glm::mat4 c3_map = get_light_pv(app_state.camera, platform_state.dir_light.direction, platform_state, ar, c2_far, app_state.camera.far_plane);

    glNamedBufferSubData(platform_state.dir_light.shadow_data.light_mats_ubo, 0, 64, glm::value_ptr(c1_map));
    glNamedBufferSubData(platform_state.dir_light.shadow_data.light_mats_ubo, 64, 64, glm::value_ptr(c2_map));
    glNamedBufferSubData(platform_state.dir_light.shadow_data.light_mats_ubo, 128, 64, glm::value_ptr(c3_map));

    glEnable(GL_DEPTH_CLAMP);

    for (size_t obj_idx = 0; obj_idx < entities.size(); ++obj_idx) {
        if (!is_set(entities.flags[obj_idx], EntityFlags::EMIT_LIGHT)) {
            translate(entities.models[obj_idx], entities.positions[obj_idx]);
            scale(entities.models[obj_idx], entities.scales[obj_idx]);
            entities.models[obj_idx].draw(shaders.dir_shadow, platform_state);
            entities.models[obj_idx].reset();
        }
    }
        
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
    glDisable(GL_DEPTH_CLAMP);

    shaders.lighting_deferred.set_f32("cascade_depths[0]", c1_far);
    shaders.lighting_deferred.set_f32("cascade_depths[1]", c2_far);
    shaders.lighting_deferred.set_f32("cascade_depths[2]", app_state.camera.far_plane);

    // ---- point lights ----

    glm::mat4 shadow_proj = glm::perspective(
        glm::radians(90.0f), (f32)platform_state.pt_shadow_data.resolution / (f32)platform_state.pt_shadow_data.resolution,
        app_state.camera.near_plane, app_state.camera.far_plane);

    std::array<glm::mat4, 6> shadow_transforms;

    glBindFramebuffer(GL_FRAMEBUFFER, platform_state.pt_shadow_data.fbo);
    glViewport(0, 0, platform_state.pt_shadow_data.resolution, platform_state.pt_shadow_data.resolution);
    glClear(GL_DEPTH_BUFFER_BIT);
    shaders.pt_shadow.set_f32("far_plane", app_state.camera.far_plane);

    for (size_t light_idx = 0; light_idx < entities.size(); ++light_idx) {
            
        if (!is_set(entities.flags[light_idx], EntityFlags::EMIT_LIGHT)) {
            continue;
        }

        glm::vec3 light_pos = entities.positions[light_idx];
            
        shadow_transforms[0] = shadow_proj * glm::lookAt(light_pos, light_pos + glm::vec3(1.0f, 0.0f, 0.0f),
                                                            glm::vec3(0.0f, -1.0f, 0.0f));
        shadow_transforms[1] = shadow_proj * glm::lookAt(light_pos, light_pos + glm::vec3(-1.0f, 0.0f, 0.0f),
                                                            glm::vec3(0.0f, -1.0f, 0.0f));
        shadow_transforms[2] = shadow_proj * glm::lookAt(light_pos, light_pos + glm::vec3(0.0f, 1.0f, 0.0f),
                                                            glm::vec3(0.0f, 0.0f, 1.0f));
        shadow_transforms[3] = shadow_proj * glm::lookAt(light_pos, light_pos + glm::vec3(0.0f, -1.0f, 0.0f),
                                                            glm::vec3(0.0f, 0.0f, -1.0f));
        shadow_transforms[4] = shadow_proj * glm::lookAt(light_pos, light_pos + glm::vec3(0.0f, 0.0f, 1.0f),
                                                            glm::vec3(0.0f, -1.0f, 0.0f));
        shadow_transforms[5] = shadow_proj * glm::lookAt(light_pos, light_pos + glm::vec3(0.0f, 0.0f, -1.0f),
                                                            glm::vec3(0.0f, -1.0f, 0.0f));

        shaders.pt_shadow.set_mat4("shadow_mats[0]", shadow_transforms[0]);
        shaders.pt_shadow.set_mat4("shadow_mats[1]", shadow_transforms[1]);
        shaders.pt_shadow.set_mat4("shadow_mats[2]", shadow_transforms[2]);
        shaders.pt_shadow.set_mat4("shadow_mats[3]", shadow_transforms[3]);
        shaders.pt_shadow.set_mat4("shadow_mats[4]", shadow_transforms[4]);
        shaders.pt_shadow.set_mat4("shadow_mats[5]", shadow_transforms[5]);
        shaders.pt_shadow.set_vec3("light_pos", light_pos);

        for (size_t obj_idx = 0; obj_idx < entities.size(); ++obj_idx) {
            if (!is_set(entities.flags[obj_idx], EntityFlags::EMIT_LIGHT)) {
                translate(entities.models[obj_idx], entities.positions[obj_idx]);
                scale(entities.models[obj_idx], entities.scales[obj_idx]);
                // for now, not casting shadows from entities that have transparency
                entities.models[obj_idx].draw(shaders.pt_shadow, platform_state, MeshFlags::TRANSPARENT, true);
                entities.models[obj_idx].reset();
            }
        }
    }

    glCullFace(GL_BACK);

    // geometry pass ==========================================================================

    gbuf_fbuf.bind();
    glNamedFramebufferDrawBuffers(gbuf_fbuf.frame_buf, 3, gbuf_fbuf.attachments.data());
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // note: we are using the stencil buffer to mask out pixels that should
    // not be impacted by lighting
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glStencilMask(0x00);

    // mask out and render skybox
    glm::mat4 static_view = view;
    static_view[3] = { 0, 0, 0, 1 }; // removing translation component
    glNamedBufferSubData(platform_state.global_ubo, 64, sizeof(glm::mat4), glm::value_ptr(static_view));
    platform_state.sky_box.draw(shaders.skybox, platform_state);
    glNamedBufferSubData(platform_state.global_ubo, 64, sizeof(glm::mat4), glm::value_ptr(view));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);

    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    glStencilMask(0xFF);

    // render non light emitters
    for (size_t idx = 0; idx < entities.size(); ++idx) {
        if (!is_set(entities.flags[idx], EntityFlags::EMIT_LIGHT)) {                
            translate(entities.models[idx], entities.positions[idx]);
            scale(entities.models[idx], entities.scales[idx]);
            entities.models[idx].draw(shaders.gbuf, platform_state, MeshFlags::TRANSPARENT, true);
            entities.models[idx].reset();
        }
    }

    // deferred pass ==========================================================================

    int_fbuf.bind();

    // compute lighting for all fragments with stencil value '1'
    glDisable(GL_DEPTH_TEST);
    glStencilFunc(GL_EQUAL, 1, 0xFF); 
    glStencilOp(GL_ZERO, GL_REPLACE, GL_REPLACE);

    shaders.lighting_deferred.set_tex("gbuf_pos", 0, gbuf_fbuf.tex_bufs[0]);
    shaders.lighting_deferred.set_tex("gbuf_norms", 1, gbuf_fbuf.tex_bufs[1]);
    shaders.lighting_deferred.set_tex("gbuf_colors", 2, gbuf_fbuf.tex_bufs[2]);
    shaders.lighting_deferred.set_tex("dir_shadow_maps", 11, platform_state.dir_light.shadow_data.tex);
    shaders.lighting_deferred.set_tex("pt_shadow_map", 12, platform_state.pt_shadow_data.tex);
    shaders.lighting_deferred.set_i32("n_cascades", platform_state.dir_light.shadow_data.n_cascades);

    int_fbuf.draw(shaders.lighting_deferred);

    // pass through for all fragments with stencil value '0'
    glStencilFunc(GL_EQUAL, 0, 0xFF); 
    glStencilOp(GL_REPLACE, GL_ZERO, GL_ZERO);
    shaders.passthrough.set_i32("gbuf_colors", 2);

    int_fbuf.draw(shaders.passthrough);

    // forward pass ===========================================================================
        
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);

    shaders.lighting_forward.set_tex("dir_shadow_maps", 11, platform_state.dir_light.shadow_data.tex);
    shaders.lighting_forward.set_tex("pt_shadow_map", 12, platform_state.pt_shadow_data.tex);
    shaders.lighting_forward.set_i32("n_cascades", platform_state.dir_light.shadow_data.n_cascades);
    shaders.lighting_forward.set_f32("cascade_depths[0]", c1_far);
    shaders.lighting_forward.set_f32("cascade_depths[1]", c2_far);
    shaders.lighting_forward.set_f32("cascade_depths[2]", app_state.camera.far_plane);

    for (size_t idx = 0; idx < entities.size(); ++idx) {
        if (is_set(entities.flags[idx], EntityFlags::EMIT_LIGHT)) {
            // draw light emitters
            translate(entities.models[idx], entities.positions[idx]);
            scale(entities.models[idx], entities.scales[idx]);
            shaders.light.set_vec4("color", entities.light_props[idx].color);
            shaders.light.set_f32("intensity", entities.light_props[idx].intensity);
            entities.models[idx].draw(shaders.light, platform_state);
            entities.models[idx].reset();
        } 
        else {
            // draw transparent components
            translate(entities.models[idx], entities.positions[idx]);
            scale(entities.models[idx], entities.scales[idx]);
            entities.models[idx].draw(shaders.lighting_forward, platform_state, MeshFlags::TRANSPARENT, false);
            entities.models[idx].reset();
        }
    }

    // post processing passes =================================================================

    // compute bloom
    if (app_state.bloom_enabled) {
        glDisable(GL_DEPTH_TEST);
        gbuf_fbuf.bind();
        glNamedFramebufferDrawBuffer(gbuf_fbuf.frame_buf, GL_COLOR_ATTACHMENT0);
        glClear(GL_COLOR_BUFFER_BIT);

        // render colors that pass brightness test
        glNamedFramebufferDrawBuffer(gbuf_fbuf.frame_buf, GL_COLOR_ATTACHMENT1);
        glClear(GL_COLOR_BUFFER_BIT);
        shaders.brightness.set_tex("tex", 0, int_fbuf.tex_bufs[0]);
        gbuf_fbuf.draw(shaders.brightness);

        bool horizontal = true;
        for (int idx = 0; idx < app_state.n_bloom_passes * 2; ++idx) {
            if (horizontal) {
                glNamedFramebufferDrawBuffer(gbuf_fbuf.frame_buf, GL_COLOR_ATTACHMENT0);
                shaders.bloom.set_tex("tex", 0, gbuf_fbuf.tex_bufs[1]);
                shaders.bloom.set_bool("horizontal", true);
                gbuf_fbuf.draw(shaders.bloom);
            } else {
                glNamedFramebufferDrawBuffer(gbuf_fbuf.frame_buf, GL_COLOR_ATTACHMENT1);
                shaders.bloom.set_tex("tex", 0, gbuf_fbuf.tex_bufs[0]);
                shaders.bloom.set_bool("horizontal", false);
                gbuf_fbuf.draw(shaders.bloom);
            }
            horizontal = !horizontal;
        }
    } 

    out_fbuf.bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    shaders.out.set_tex("bloom_tex", 0, gbuf_fbuf.tex_bufs[1]);
    shaders.out.set_tex("tex", 1, int_fbuf.tex_bufs[0]);
    shaders.out.set_bool("bloom_enabled", app_state.bloom_enabled);
    shaders.out.set_f32("gamma", app_state.window_state.gamma);
    shaders.out.set_f32("exposure", app_state.window_state.exposure);

    out_fbuf.draw(shaders.out);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gui::gl_imgui(app_state, *this);
};

void GL_Platform::finish() { 
    ImGui_ImplOpenGL3_Shutdown();
};

void GL_Platform::enable_vsync(bool enable) { (enable) ? glfwSwapInterval(1) : glfwSwapInterval(0); };


} // namespace rose