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

std::optional<rses> GL_Platform::init(AppData& app_data) {

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
    
    EntityCtx model1_def = { .model_pth = SOURCE_DIR "/assets/model1/model1.obj",
                             .pos = { 0.0f, 3.0f, 0.0f },
                             .scale = { 1.0f, 1.0f, 1.0f },
                             .light_props = PtLight(),
                             .flags = EntityFlags::NONE 
                           };

    entities.add_object(texture_manager, model1_def);

    EntityCtx model2_def = { .model_pth = SOURCE_DIR "/assets/model1/model1.obj",
                             .pos = { 0.0f, 7.0f, 0.0f },
                             .scale = { 1.0f, 1.0f, 1.0f },
                             .light_props = PtLight(),
                             .flags = EntityFlags::NONE };

    entities.add_object(texture_manager, model2_def);

    EntityCtx model3_def = { .model_pth = SOURCE_DIR "/assets/model1/model1.obj",
                             .pos = { 0.0f, 10.0f, 0.0f },
                             .scale = { 1.0f, 1.0f, 1.0f },
                             .light_props = PtLight(),
                             .flags = EntityFlags::EMIT_LIGHT };

    entities.add_object(texture_manager, model3_def);
    entities.light_props.back().color = { 0.35f, 0.1f, 0.1f, 1.0f };

    // frame buf initialization ===================================================================
    
    // TODO: optimize size of framebuffers

    // (position, normal, albedo)
    if (err = gbuf.init(app_data.window_data.width, app_data.window_data.height, true, { { GL_RGBA16F }, { GL_RGBA16F }, { GL_RGBA8 } })) {
        return err;
    }

    if (err = pp1.init(app_data.window_data.width, app_data.window_data.height, false, { { GL_RGBA16F }, { GL_RGBA16F } })) {
        return err;
    }

    // note: pp1 uses the render buffer of gbuf to perform masking with the stencil buffer
    glNamedFramebufferRenderbuffer(pp1.frame_buf, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gbuf.render_buf);

    if (err = fbuf_out.init(app_data.window_data.width, app_data.window_data.height, false, { { GL_RGBA8 } })) {
        return err;
    }

    // uniform buffer initialization ==============================================================

    glCreateBuffers(1, &platform_state.global_ubo);
    glNamedBufferStorage(platform_state.global_ubo, 208, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, platform_state.global_ubo);

    glm::uvec2 screen_dims = { app_data.window_data.width, app_data.window_data.height };
    glNamedBufferSubData(platform_state.global_ubo, 176, 16, glm::value_ptr(clusters.grid_sz));
    glNamedBufferSubData(platform_state.global_ubo, 192, 8, glm::value_ptr(screen_dims));
    glNamedBufferSubData(platform_state.global_ubo, 200, 4, &app_data.camera.far_plane);
    glNamedBufferSubData(platform_state.global_ubo, 204, 4, &app_data.camera.near_plane);

    // ssbo initialization ========================================================================

    i32 n_clusters = clusters.grid_sz.x * clusters.grid_sz.y * clusters.grid_sz.z;
    clusters.clusters_aabb_ssbo.init(sizeof(AABB) * n_clusters, 2);
    clusters.lights_ssbo.init(sizeof(PtLight) * 1024, 3);
    clusters.lights_pos_ssbo.init(sizeof(glm::vec4) * 1024, 4);
    update_light_ssbos(entities, clusters);
    clusters.clusters_ssbo.init(sizeof(u32) * (1 + clusters.max_lights_in_cluster) * n_clusters, 5);

    // shadow map initialization ==================================================================
    
    // ---- directional shadow map ----

    glCreateFramebuffers(1, &platform_state.dir_light.shadow.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, platform_state.dir_light.shadow.fbo);
    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &platform_state.dir_light.shadow.tex);

    // TODO: Resolution could vary between cascades to save memory
    glTextureStorage3D(platform_state.dir_light.shadow.tex, 1, GL_DEPTH_COMPONENT32F, 
                       platform_state.dir_light.shadow.resolution,
                       platform_state.dir_light.shadow.resolution, platform_state.dir_light.shadow.n_cascades);

    glTextureParameteri(platform_state.dir_light.shadow.tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(platform_state.dir_light.shadow.tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(platform_state.dir_light.shadow.tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(platform_state.dir_light.shadow.tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(platform_state.dir_light.shadow.tex, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    glNamedFramebufferTexture(platform_state.dir_light.shadow.fbo, GL_DEPTH_ATTACHMENT, platform_state.dir_light.shadow.tex, 0);
    glNamedFramebufferDrawBuffer(platform_state.dir_light.shadow.fbo, GL_NONE);
    glNamedFramebufferReadBuffer(platform_state.dir_light.shadow.fbo, GL_NONE);

    if (glCheckNamedFramebufferStatus(platform_state.dir_light.shadow.fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        return rses().gl("directional shadow framebuffer is incomplete");
    }

    glCreateBuffers(1, &platform_state.dir_light.light_mats_ubo);
    glNamedBufferStorage(platform_state.dir_light.light_mats_ubo, 192, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_UNIFORM_BUFFER, 6, platform_state.dir_light.light_mats_ubo);

    // ---- point shadow map ----

    glCreateFramebuffers(1, &platform_state.pt_shadow.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, platform_state.pt_shadow.fbo);
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &platform_state.pt_shadow.tex);

    glTextureStorage2D(platform_state.pt_shadow.tex, 1, GL_DEPTH_COMPONENT32F, platform_state.pt_shadow.resolution,
                       platform_state.pt_shadow.resolution);

    glTextureParameteri(platform_state.pt_shadow.tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(platform_state.pt_shadow.tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(platform_state.pt_shadow.tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(platform_state.pt_shadow.tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(platform_state.pt_shadow.tex, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glNamedFramebufferTexture(platform_state.pt_shadow.fbo, GL_DEPTH_ATTACHMENT, platform_state.pt_shadow.tex, 0);
    glNamedFramebufferDrawBuffer(platform_state.pt_shadow.fbo, GL_NONE);
    glNamedFramebufferReadBuffer(platform_state.pt_shadow.fbo, GL_NONE);
    
    if (glCheckNamedFramebufferStatus(platform_state.pt_shadow.fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        return rses().gl("point shadow framebuffer is incomplete");
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return std::nullopt;
};

// obtains a projection-view matrix for a directional light
// near and far values should be specific to the cascade
static glm::mat4 get_light_pv(const Camera& camera, const glm::vec3& light_dir, const GL_PlatformState& state, float ar, f32 near, f32 far)
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

    glm::vec3 frust_center;
    // [ clip -> world ]
    for (auto& corner : frustum_corners) {        
        corner = pv_inv * corner;
        corner /= corner.w;
        frust_center += glm::vec3(corner);
    }

    frust_center /= frustum_corners.size();
    glm::mat4 light_view = glm::lookAt(frust_center - light_dir * 10.0f, frust_center, { 0.0f, 1.0f, 0.0f });

    // find tight bounds for the frustum
    glm::vec3 min_pt = constants::vec3_max;
    glm::vec3 max_pt = constants::vec3_min;
    for (const auto& corner : frustum_corners) {
        glm::vec4 pt = light_view * corner; // [ world -> light view ]
        min_pt = { std::min(min_pt.x, pt.x), std::min(min_pt.y, pt.y), std::min(min_pt.z, pt.z) };
        max_pt = { std::max(max_pt.x, pt.x), std::max(max_pt.y, pt.y), std::max(max_pt.z, pt.z) };
    }

    glm::mat4 light_proj = glm::ortho(min_pt.x, max_pt.x, min_pt.y, max_pt.y, -max_pt.z, -min_pt.z);
    return light_proj * light_view;
}

void GL_Platform::update(AppData& app_data) {
   
    // frame set up ===============================================================================================
        
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
        
    ImGuiIO& io = ImGui::GetIO();

    // TODO: this is only used at a single point in the gui code
    // should find a better solution
    app_data.window_data.dock_id = ImGui::DockSpaceOverViewport();

    glEnable(GL_DEPTH_TEST);

    f32 ar = (f32)app_data.window_data.width / (f32)app_data.window_data.height;
    glm::mat4 projection = app_data.camera.projection(ar);
    glm::mat4 view = app_data.camera.view();

    // update ubo state
    glNamedBufferSubData(platform_state.global_ubo, 0, 64, glm::value_ptr(projection));
    glNamedBufferSubData(platform_state.global_ubo, 64, 64, glm::value_ptr(view));
    glNamedBufferSubData(platform_state.global_ubo, 128, 16, glm::value_ptr(app_data.camera.position));
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
    shaders.clusters_cull.set_int("n_lights", clusters.lights_ssbo.n_elems);

    glDispatchCompute(27, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // shadow pass ================================================================================================
        
    glCullFace(GL_FRONT);  // prevent peter panning

    // ---- directional light ----

    glBindFramebuffer(GL_FRAMEBUFFER, platform_state.dir_light.shadow.fbo);
    glViewport(0, 0, platform_state.dir_light.shadow.resolution, platform_state.dir_light.shadow.resolution);
    glClear(GL_DEPTH_BUFFER_BIT);

    f32 c1_far = 10.0f;
    f32 c2_far = 30.0f;

    glm::mat4 c1_map = get_light_pv(app_data.camera, platform_state.dir_light.direction, platform_state, ar, app_data.camera.near_plane, c1_far);
    glm::mat4 c2_map = get_light_pv(app_data.camera, platform_state.dir_light.direction, platform_state, ar, c1_far, c2_far);
    glm::mat4 c3_map = get_light_pv(app_data.camera, platform_state.dir_light.direction, platform_state, ar, c2_far, app_data.camera.far_plane);

    glNamedBufferSubData(platform_state.dir_light.light_mats_ubo, 0, 64, glm::value_ptr(c1_map));
    glNamedBufferSubData(platform_state.dir_light.light_mats_ubo, 64, 64, glm::value_ptr(c2_map));
    glNamedBufferSubData(platform_state.dir_light.light_mats_ubo, 128, 64, glm::value_ptr(c3_map));

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

    shaders.lighting_deferred.set_float("cascade_depths[0]", c1_far);
    shaders.lighting_deferred.set_float("cascade_depths[1]", c2_far);
    shaders.lighting_deferred.set_float("cascade_depths[2]", app_data.camera.far_plane);

    // ---- point lights ----

    glm::mat4 shadow_proj = glm::perspective(
        glm::radians(90.0f), (f32)platform_state.pt_shadow.resolution / (f32)platform_state.pt_shadow.resolution,
        app_data.camera.near_plane, app_data.camera.far_plane);

    std::array<glm::mat4, 6> shadow_transforms;

    glBindFramebuffer(GL_FRAMEBUFFER, platform_state.pt_shadow.fbo);
    glViewport(0, 0, platform_state.pt_shadow.resolution, platform_state.pt_shadow.resolution);
    glClear(GL_DEPTH_BUFFER_BIT);
    shaders.pt_shadow.set_float("far_plane", app_data.camera.far_plane);

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

    gbuf.bind();
    glNamedFramebufferDrawBuffers(gbuf.frame_buf, 3, gbuf.attachments.data());
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

    pp1.bind();
    glNamedFramebufferDrawBuffers(pp1.frame_buf, 2, pp1.attachments.data());

    // compute lighting for all fragments with stencil value '1'
    glDisable(GL_DEPTH_TEST);
    glStencilFunc(GL_EQUAL, 1, 0xFF); 
    glStencilOp(GL_ZERO, GL_REPLACE, GL_REPLACE);

    shaders.lighting_deferred.set_tex("gbuf_pos", 0, gbuf.tex_bufs[0]);
    shaders.lighting_deferred.set_tex("gbuf_norms", 1, gbuf.tex_bufs[1]);
    shaders.lighting_deferred.set_tex("gbuf_colors", 2, gbuf.tex_bufs[2]);
    shaders.lighting_deferred.set_tex("dir_shadow_maps", 11, platform_state.dir_light.shadow.tex);
    shaders.lighting_deferred.set_tex("pt_shadow_map", 12, platform_state.pt_shadow.tex);
    shaders.lighting_deferred.set_int("n_cascades", platform_state.dir_light.shadow.n_cascades);

    pp1.draw(shaders.lighting_deferred);

    // pass through for all fragments with stencil value '0'
    glStencilFunc(GL_EQUAL, 0, 0xFF); 
    glStencilOp(GL_REPLACE, GL_ZERO, GL_ZERO);
    shaders.passthrough.set_int("gbuf_colors", 2);

    pp1.draw(shaders.passthrough);

    // forward pass ===========================================================================
        
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);

    shaders.lighting_forward.set_tex("dir_shadow_maps", 11, platform_state.dir_light.shadow.tex);
    shaders.lighting_forward.set_tex("pt_shadow_map", 12, platform_state.pt_shadow.tex);
    shaders.lighting_forward.set_int("n_cascades", platform_state.dir_light.shadow.n_cascades);
    shaders.lighting_forward.set_float("cascade_depths[0]", c1_far);
    shaders.lighting_forward.set_float("cascade_depths[1]", c2_far);
    shaders.lighting_forward.set_float("cascade_depths[2]", app_data.camera.far_plane);

    for (size_t idx = 0; idx < entities.size(); ++idx) {
        if (is_set(entities.flags[idx], EntityFlags::EMIT_LIGHT)) {
            // draw light emitters
            translate(entities.models[idx], entities.positions[idx]);
            scale(entities.models[idx], entities.scales[idx]);
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
    if (app_data.bloom_on) {
        glNamedFramebufferDrawBuffer(pp1.frame_buf, GL_COLOR_ATTACHMENT1);
        glClear(GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        bool horizontal = false;
        shaders.blur.set_tex("tex", 0, pp1.tex_bufs[1]);
        pp1.draw(shaders.blur);

        for (int i = 0; i < app_data.n_bloom_passes * 2 - 1; ++i) {
            if (horizontal) {
                pp1.bind();
                glNamedFramebufferDrawBuffer(pp1.frame_buf, GL_COLOR_ATTACHMENT1);
                shaders.blur.set_tex("tex", 0, gbuf.tex_bufs[0]);
                shaders.blur.set_bool("horizontal", true);
                pp1.draw(shaders.blur);
            } else {
                // note: reusing the gbuf position buf for bloom calculations
                gbuf.bind();
                glNamedFramebufferDrawBuffer(gbuf.frame_buf, GL_COLOR_ATTACHMENT0);
                shaders.blur.set_tex("tex", 0, pp1.tex_bufs[1]);
                shaders.blur.set_bool("horizontal", false);
                gbuf.draw(shaders.blur);
            }
            horizontal = !horizontal;
        }
    }

    fbuf_out.bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    shaders.hdr.set_tex("scene_tex", 0, pp1.tex_bufs[0]);
    shaders.hdr.set_tex("blur_tex", 1, pp1.tex_bufs[1]);
    shaders.hdr.set_float("gamma", app_data.window_data.gamma);
    shaders.hdr.set_float("exposure", app_data.window_data.exposure);
    shaders.hdr.set_bool("bloom_enabled", app_data.bloom_on);

    fbuf_out.draw(shaders.hdr);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gui::gl_imgui(app_data, *this);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
};

void GL_Platform::finish() { 
    ImGui_ImplOpenGL3_Shutdown();
};

void GL_Platform::enable_vsync(bool enable) { (enable) ? glfwSwapInterval(1) : glfwSwapInterval(0); };


} // namespace rose