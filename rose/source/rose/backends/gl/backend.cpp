#include <rose/gui.hpp>
#include <rose/model.hpp>
#include <rose/core/err.hpp>
#include <rose/backends/gl/backend.hpp>
#include <rose/backends/gl/render.hpp>
#include <rose/backends/gl/structs.hpp>

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
#include <random>

namespace gl {

static void GLAPIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei len,
                                  const GLchar* msg, const void* user_param) {
    std::println("GL ERROR: type = {}, severity = {}, message = {}\n", type, severity, msg);
}

static rses init_gl() {
    if (GLenum glew_success = glewInit(); glew_success != GLEW_OK) {
        return rses().gl(std::format("GLEW failed to initialize: {}", (const char*)glewGetErrorString(glew_success)));
    }

    std::println("GLEW successfully initialized version: {}", (const char*)glewGetString(GLEW_VERSION));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

#ifdef _DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, NULL, GL_FALSE);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
    glDebugMessageCallback(gl_debug_callback, nullptr);
#endif

    return {};
}

rses Backend::init(AppState& app_state) {

    if (auto err = init_gl()) {
        return err.general("unable to initialize GLEW and OpenGL");
    }

    if (auto err = shaders.init()) {
        return err.general("unable to initialize shaders");
    }

    texture_manager.init();

    backend_state.skybox.init();
    backend_state.skybox.load(texture_manager,
                            { SOURCE_DIR "/assets/skybox/right.jpg", SOURCE_DIR "/assets/skybox/left.jpg",
                            SOURCE_DIR "/assets/skybox/top.jpg", SOURCE_DIR "/assets/skybox/bottom.jpg",
                            SOURCE_DIR "/assets/skybox/front.jpg", SOURCE_DIR "/assets/skybox/back.jpg" });

    // note: hard coding some model loading for testing, can be removed eventually
    EntityCtx sponza_def = { .model_path = SOURCE_DIR "/assets/Sponza/glTF/Sponza.gltf",
                             .pos = { 0.0f, 0.0f, 0.0f },
                             .scale = { 0.02f, 0.02f, 0.02f },
                             .rotation = { 0.0f, 0.0f, 0.0f },
                             .light_data = PtLight(),
                             .flags = EntityFlags::NONE };

    app_state.entities.add_object(texture_manager, sponza_def);

    EntityCtx model2_def = { .model_path = SOURCE_DIR "/assets/sphere/scene.gltf",
                             .pos = { 0.0f, 3.5f, 0.0f },
                             .scale = { 0.1f, 0.1f, 0.1f },
                             .rotation = { 0.0f, 0.0f, 0.0f },
                             .light_data = PtLight(),
                             .flags = EntityFlags::NONE };

    app_state.entities.add_object(texture_manager, model2_def);

    // frame buf initialization ===================================================================

    // TODO: optimize size of framebuffers

    // (position, normal, albedo)
    if (auto err = gbuf_fbuf.init(app_state.window_state.width, app_state.window_state.height, true,
                             { { GL_RGBA16F }, { GL_RGB16F }, { GL_RGBA8 } })) {
        return err;
    }

    if (auto err = int_fbuf.init(app_state.window_state.width, app_state.window_state.height, false, { { GL_RGBA16F } })) {
        return err;
    }

    // note: pp1 uses the render buffer of gbuf to perform masking with the stencil buffer
    glNamedFramebufferRenderbuffer(int_fbuf.frame_buf, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                   gbuf_fbuf.render_buf);

    if (auto err = ssao_fbuf.init(app_state.window_state.width, app_state.window_state.height, false, {{ GL_R16F }, { GL_R16F }})) {
        return err;
    }

    if (auto err = out_fbuf.init(app_state.window_state.width, app_state.window_state.height, false, { { GL_RGBA8 } })) {
        return err;
    }

    // uniform buffer initialization ==============================================================

    glCreateBuffers(1, &backend_state.global_ubo);
    glNamedBufferStorage(backend_state.global_ubo, 176, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, backend_state.global_ubo);
    glm::uvec2 screen_dims = { app_state.window_state.width, app_state.window_state.height };    
    glNamedBufferSubData(backend_state.global_ubo, 144, 16, glm::value_ptr(clusters.grid_sz));
    glNamedBufferSubData(backend_state.global_ubo, 160, 8, glm::value_ptr(screen_dims));
    glNamedBufferSubData(backend_state.global_ubo, 168, 4, &app_state.camera.far_plane);
    glNamedBufferSubData(backend_state.global_ubo, 172, 4, &app_state.camera.near_plane);

    // ssbo initialization ========================================================================

    i32 n_clusters = clusters.grid_sz.x * clusters.grid_sz.y * clusters.grid_sz.z;
    clusters.gl_data.aabb_ssbo.init(sizeof(AABB) * n_clusters, 2);
    clusters.gl_data.lights_ssbo.init(sizeof(PtLight) * 1024, 3);
    clusters.gl_data.lights_pos_ssbo.init(sizeof(glm::vec4) * 1024, 4);
    lights_ids_ssbo.init(sizeof(u32) * 1024, 7);
    clusters.gl_data.clusters_ssbo.init(sizeof(u32) * (1 + clusters.max_lights_in_cluster) * n_clusters, 5);

    // shadow map initialization ==================================================================

    // ---- directional shadow map ----

    if (auto err = backend_state.dir_light.gl_shadow.init()) {
        return err;
    }

    // ---- point shadow map ----

    if (auto err = backend_state.pt_shadow_data.init()) {
        return err;
    }

    // SSAO initialization ========================================================================

    // create random noise to rotate SSAO hemisphere along tangest space z-axis
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<f32> dist(0.0f, 1.0f);

    std::vector<glm::vec4> ssao_noise;
    ssao_noise.resize(16);

    for (u32 idx = 0; idx < ssao_noise.size(); ++idx) {
        glm::vec4 noise = glm::vec4(dist(rng) * 2.0f - 1.0f, dist(rng) * 2.0f - 1.0f, 0.0f, 1.0f);
        ssao_noise[idx] = noise;
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &backend_state.ssao_noise_tex);
    glTextureStorage2D(backend_state.ssao_noise_tex, 1, GL_RGBA16F, 4, 4);
    glTextureSubImage2D(backend_state.ssao_noise_tex, 0, 0, 0, 4, 4, GL_RGBA, GL_FLOAT, ssao_noise.data());
    glTextureParameteri(backend_state.ssao_noise_tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(backend_state.ssao_noise_tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(backend_state.ssao_noise_tex, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(backend_state.ssao_noise_tex, GL_TEXTURE_WRAP_T, GL_REPEAT);

    backend_state.ssao_samples_ssbo.init(sizeof(glm::vec4) * app_state.ssao_kernel.size(), 10);
    backend_state.ssao_samples_ssbo.update(std::span(app_state.ssao_kernel.begin(), app_state.ssao_kernel.end()));

    // remaining set up ===========================================================================

    shaders.skybox.set_vec3("dir_light.direction", backend_state.dir_light.direction);
    shaders.skybox.set_vec3("dir_light.color", backend_state.dir_light.color);
    shaders.skybox.set_f32("dir_light.ambient_strength", backend_state.dir_light.ambient_strength);

    shaders.lighting_deferred.set_u32("pt_caster_id", 0);
    shaders.lighting_deferred.set_bool("ao_enabled", app_state.ssao_enabled);
    shaders.lighting_deferred.set_vec3("dir_light.direction", backend_state.dir_light.direction);
    shaders.lighting_deferred.set_vec3("dir_light.color", backend_state.dir_light.color);
    shaders.lighting_deferred.set_f32("dir_light.ambient_strength", backend_state.dir_light.ambient_strength);
    
    shaders.lighting_forward.set_u32("pt_caster_id", 0);
    shaders.lighting_forward.set_vec3("dir_light.direction", backend_state.dir_light.direction);
    shaders.lighting_forward.set_vec3("dir_light.color", backend_state.dir_light.color);
    shaders.lighting_forward.set_f32("dir_light.ambient_strength", backend_state.dir_light.ambient_strength);

    backend_state.bloom_mip_chain = create_mip_chain(app_state.window_state.width, app_state.window_state.height, 5);
    f32 ar = (f32)app_state.window_state.width / (f32)app_state.window_state.height;
    glm::vec2 filter_sz = { 0.005f, 0.005f * ar };
    shaders.upsample.set_vec2("filter_sz", filter_sz);
    shaders.out.set_bool("bloom_enabled", app_state.bloom_enabled);

    return {};
};

void Backend::new_frame(AppState& app_state) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    app_state.window_state.dock_id = ImGui::DockSpaceOverViewport();
}

void Backend::end_frame(GLFWwindow* window_handle) {
    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}

void Backend::step(AppState& app_state) {

    // frame set up ===============================================================================================

    glEnable(GL_DEPTH_TEST);
    Entities& entities = app_state.entities;

    f32 ar = (f32)app_state.window_state.width / (f32)app_state.window_state.height;
    glm::mat4 projection = app_state.camera.projection(ar);
    glm::mat4 view = app_state.camera.view();

    // update ubo state
    glNamedBufferSubData(backend_state.global_ubo, 0, 64, glm::value_ptr(projection));
    glNamedBufferSubData(backend_state.global_ubo, 64, 64, glm::value_ptr(view));
    glNamedBufferSubData(backend_state.global_ubo, 128, 16, glm::value_ptr(app_state.camera.position));

    // clustered set-up ===========================================================================================

    // determine the AABB for each cluster
    // note: this does not need to be computed on every frame if parameters have not changed
    shaders.clusters_build.use();
    shaders.clusters_build.set_mat4("inv_proj", glm::inverse(projection));

    glDispatchCompute(clusters.grid_sz.x, clusters.grid_sz.y, clusters.grid_sz.z);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // build light lists for each cluster
    shaders.clusters_cull.use();
    shaders.clusters_cull.set_i32("n_lights", clusters.gl_data.lights_ssbo.n_elems);

    glDispatchCompute(27, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // shadow pass ================================================================================================

    glCullFace(GL_FRONT); // prevent peter panning

    // ---- directional light ----

    glBindFramebuffer(GL_FRAMEBUFFER, backend_state.dir_light.gl_shadow.fbo);
    glViewport(0, 0, backend_state.dir_light.gl_shadow.resolution, backend_state.dir_light.gl_shadow.resolution);
    glClear(GL_DEPTH_BUFFER_BIT);

    // cascades: [0.1, 10.0], [10.0, 30.0], [30.0, 100.0]
    f32 c1_far = 10.0f;
    f32 c2_far = 30.0f;

    auto p1 = glm::perspective(glm::radians(app_state.camera.zoom), ar, app_state.camera.near_plane, c1_far);
    auto p2 = glm::perspective(glm::radians(app_state.camera.zoom), ar, c1_far, c2_far);
    auto p3 = glm::perspective(glm::radians(app_state.camera.zoom), ar, c2_far, app_state.camera.far_plane);

    glm::mat4 c1_map = get_cascade_mat(p1, app_state.camera.view(), backend_state.dir_light.direction,
                                       backend_state.dir_light.gl_shadow.resolution);
    glm::mat4 c2_map = get_cascade_mat(p2, app_state.camera.view(), backend_state.dir_light.direction,
                                       backend_state.dir_light.gl_shadow.resolution);
    glm::mat4 c3_map = get_cascade_mat(p3, app_state.camera.view(), backend_state.dir_light.direction,
                                       backend_state.dir_light.gl_shadow.resolution);

    glNamedBufferSubData(backend_state.dir_light.gl_shadow.light_mats_ubo, 0, 64, glm::value_ptr(c1_map));
    glNamedBufferSubData(backend_state.dir_light.gl_shadow.light_mats_ubo, 64, 64, glm::value_ptr(c2_map));
    glNamedBufferSubData(backend_state.dir_light.gl_shadow.light_mats_ubo, 128, 64, glm::value_ptr(c3_map));

    glEnable(GL_DEPTH_CLAMP);

    // render occluders
    for (size_t obj_idx = 0; obj_idx < entities.size(); ++obj_idx) {
        if (entities.is_alive(obj_idx) && !entities.is_light(obj_idx)) {
            translate(entities.models[obj_idx], entities.positions[obj_idx]);
            scale(entities.models[obj_idx], entities.scales[obj_idx]);
            rotate(entities.models[obj_idx], entities.rotations[obj_idx]);
            render(shaders.dir_shadow, entities.models[obj_idx]);
            entities.models[obj_idx].reset();
        }
    }

    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
    glDisable(GL_DEPTH_CLAMP);

    // ---- point lights ----

    glm::mat4 shadow_proj =
        glm::perspective(glm::radians(90.0f),
                         (f32)backend_state.pt_shadow_data.resolution / (f32)backend_state.pt_shadow_data.resolution,
                         app_state.camera.near_plane, app_state.camera.far_plane);

    std::array<glm::mat4, 6> shadow_transforms;

    glBindFramebuffer(GL_FRAMEBUFFER, backend_state.pt_shadow_data.fbo);
    glViewport(0, 0, backend_state.pt_shadow_data.resolution, backend_state.pt_shadow_data.resolution);
    glClear(GL_DEPTH_BUFFER_BIT);
    shaders.pt_shadow.set_f32("far_plane", app_state.camera.far_plane);

    if (!entities.empty() && entities.is_alive(entities.pt_caster_idx) && entities.is_light(entities.pt_caster_idx)) {
        glm::vec3 light_pos = entities.positions[entities.pt_caster_idx];

        shadow_transforms[0] =
            shadow_proj * glm::lookAt(light_pos, light_pos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        shadow_transforms[1] = shadow_proj * glm::lookAt(light_pos, light_pos + glm::vec3(-1.0f, 0.0f, 0.0f),
                                                         glm::vec3(0.0f, -1.0f, 0.0f));
        shadow_transforms[2] =
            shadow_proj * glm::lookAt(light_pos, light_pos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        shadow_transforms[3] = shadow_proj * glm::lookAt(light_pos, light_pos + glm::vec3(0.0f, -1.0f, 0.0f),
                                                         glm::vec3(0.0f, 0.0f, -1.0f));
        shadow_transforms[4] =
            shadow_proj * glm::lookAt(light_pos, light_pos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
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
            if (entities.is_alive(obj_idx) && !entities.is_light(obj_idx)) {
                translate(entities.models[obj_idx], entities.positions[obj_idx]);
                scale(entities.models[obj_idx], entities.scales[obj_idx]);
                rotate(entities.models[obj_idx], entities.rotations[obj_idx]);
                render_opaque(shaders.pt_shadow, entities.models[obj_idx]);
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
    shaders.skybox.set_mat4("static_view", static_view);
    render(shaders.skybox, backend_state.skybox, backend_state.skybox.vao);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);

    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    glStencilMask(0xFF);

    // render non light emitters
    for (size_t idx = 0; idx < entities.size(); ++idx) {
        if (entities.is_alive(idx) && !entities.is_light(idx)) {
            translate(entities.models[idx], entities.positions[idx]);
            scale(entities.models[idx], entities.scales[idx]);
            rotate(entities.models[idx], entities.rotations[idx]);
            render_opaque(shaders.gbuf, entities.models[idx]);
            entities.models[idx].reset();
        }
    }

    // compute ambient occlusion ==============================================================
    
    if (app_state.ssao_enabled) {
        // render occlusion texture
        ssao_fbuf.bind();
        glNamedFramebufferDrawBuffer(ssao_fbuf.frame_buf, GL_COLOR_ATTACHMENT0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glm::vec2 noise_scale = glm::vec2((f32)app_state.window_state.width / 4.0f, (f32)app_state.window_state.height / 4.0f);
        shaders.ssao.set_tex("gbuf_pos", 0, gbuf_fbuf.tex_bufs[0]);
        shaders.ssao.set_tex("gbuf_norms", 1, gbuf_fbuf.tex_bufs[1]);
        shaders.ssao.set_tex("noise_tex", 2, backend_state.ssao_noise_tex);
        shaders.ssao.set_vec2("noise_scale", noise_scale);
        ssao_fbuf.draw(shaders.ssao);
        // blur the output
        glNamedFramebufferDrawBuffer(ssao_fbuf.frame_buf, GL_COLOR_ATTACHMENT1);
        shaders.blur.set_tex("occlusion_tex", 0, ssao_fbuf.tex_bufs[0]);
        ssao_fbuf.draw(shaders.blur);
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
    shaders.lighting_deferred.set_tex("occlusion_tex", 3, ssao_fbuf.tex_bufs[1]);
    shaders.lighting_deferred.set_tex("dir_shadow_maps", 11, backend_state.dir_light.gl_shadow.tex);
    shaders.lighting_deferred.set_tex("pt_shadow_map", 12, backend_state.pt_shadow_data.tex);
    shaders.lighting_deferred.set_i32("n_cascades", backend_state.dir_light.gl_shadow.n_cascades);
    shaders.lighting_deferred.set_f32("cascade_depths[0]", c1_far);
    shaders.lighting_deferred.set_f32("cascade_depths[1]", c2_far);
    shaders.lighting_deferred.set_f32("cascade_depths[2]", app_state.camera.far_plane);

    int_fbuf.draw(shaders.lighting_deferred);

    // pass through for all fragments with stencil value '0'
    glStencilFunc(GL_EQUAL, 0, 0xFF);
    glStencilOp(GL_REPLACE, GL_ZERO, GL_ZERO);
    shaders.passthrough.set_i32("gbuf_colors", 2);

    int_fbuf.draw(shaders.passthrough);

    // forward pass ===========================================================================

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);

    shaders.lighting_forward.set_tex("dir_shadow_maps", 11, backend_state.dir_light.gl_shadow.tex);
    shaders.lighting_forward.set_tex("pt_shadow_map", 12, backend_state.pt_shadow_data.tex);
    shaders.lighting_forward.set_i32("n_cascades", backend_state.dir_light.gl_shadow.n_cascades);
    shaders.lighting_forward.set_f32("cascade_depths[0]", c1_far);
    shaders.lighting_forward.set_f32("cascade_depths[1]", c2_far);
    shaders.lighting_forward.set_f32("cascade_depths[2]", app_state.camera.far_plane);

    for (size_t idx = 0; idx < entities.size(); ++idx) {
        if (!entities.is_alive(idx)) {
            continue;
        }
        if (entities.is_light(idx)) {
            // draw light emitters
            translate(entities.models[idx], entities.positions[idx]);
            scale(entities.models[idx], entities.scales[idx]);
            rotate(entities.models[idx], entities.rotations[idx]);
            shaders.light.set_vec4("color", entities.light_data[idx].color);
            shaders.light.set_f32("intensity", entities.light_data[idx].intensity);
            render(shaders.light, entities.models[idx]);
            entities.models[idx].reset();
        } 
        else {
            // draw transparent components
            translate(entities.models[idx], entities.positions[idx]);
            scale(entities.models[idx], entities.scales[idx]);
            rotate(entities.models[idx], entities.rotations[idx]);
            render_transparent(shaders.lighting_forward, entities.models[idx]);
            entities.models[idx].reset();
        }
    }

    // post processing ========================================================================

    // compute bloom
    if (app_state.bloom_enabled) {
        gbuf_fbuf.bind();
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);
        glNamedFramebufferDrawBuffer(gbuf_fbuf.frame_buf, GL_COLOR_ATTACHMENT0);

        glm::vec2 init_sz = { (f32)app_state.window_state.width, (f32)app_state.window_state.height };
        shaders.downsample.set_tex("tex", 0, int_fbuf.tex_bufs[0]);
        shaders.downsample.set_vec2("tex_sz", init_sz);

        // downsample mips
        for (size_t idx = 0; idx < backend_state.bloom_mip_chain.size(); ++idx) {
            const Mip& mip = backend_state.bloom_mip_chain[idx];
            glViewport(0, 0, (int)mip.sz.x, (int)mip.sz.y);
            glNamedFramebufferTexture(gbuf_fbuf.frame_buf, GL_COLOR_ATTACHMENT0, mip.tex, 0);
            gbuf_fbuf.draw(shaders.downsample);
            shaders.downsample.set_tex("tex", 0, mip.tex);
            shaders.downsample.set_vec2("texel_size", 1.0f / mip.sz);
        }

        // upsample + blur mips
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glBlendEquation(GL_FUNC_ADD);

        for (size_t idx = backend_state.bloom_mip_chain.size() - 1; idx > 0; --idx) {
            const Mip& mip = backend_state.bloom_mip_chain[idx];
            const Mip& next_mip = backend_state.bloom_mip_chain[idx-1];
            shaders.upsample.set_tex("tex", 0, mip.tex);
            glViewport(0, 0, (int)next_mip.sz.x, (int)next_mip.sz.y);
            glNamedFramebufferTexture(gbuf_fbuf.frame_buf, GL_COLOR_ATTACHMENT0, next_mip.tex, 0);
            gbuf_fbuf.draw(shaders.upsample);
        }

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);

        shaders.upsample.set_tex("tex", 0, backend_state.bloom_mip_chain[0].tex);
        glViewport(0, 0, app_state.window_state.width, app_state.window_state.height);
        glNamedFramebufferTexture(gbuf_fbuf.frame_buf, GL_COLOR_ATTACHMENT0, gbuf_fbuf.tex_bufs[0], 0);
        gbuf_fbuf.draw(shaders.upsample);
    }

    // render final image
    out_fbuf.bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    
    shaders.out.set_tex("tex", 0, int_fbuf.tex_bufs[0]);
    shaders.out.set_tex("bloom_tex", 1, gbuf_fbuf.tex_bufs[0]);
    shaders.out.set_f32("bloom_factor", app_state.bloom_factor);
    shaders.out.set_f32("gamma", app_state.window_state.gamma);
    shaders.out.set_f32("exposure", app_state.window_state.exposure);

    out_fbuf.draw(shaders.out);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glNamedFramebufferTexture(gbuf_fbuf.frame_buf, GL_COLOR_ATTACHMENT0, gbuf_fbuf.tex_bufs[0], 0);
    
    // gui pass ===================================================================================

    gui::GuiRet gui_ret = gui::imgui(app_state, *this);

    // if a light was changed within the gui, we need to update our GPU buffers to reflect these changes
    if (gui_ret.light_changed) {
        static std::vector<PtLight> pt_light_data;
        static std::vector<glm::vec4> pt_lights_pos;
        static std::vector<u32> pt_lights_ids;

        // note: linear search, probably not a big deal for now
        for (size_t idx = 0; idx < entities.size(); ++idx) {
            if (entities.is_alive(idx) && entities.is_light(idx)) {
                pt_lights_pos.push_back(glm::vec4(entities.positions[idx], 1.0f));
                pt_light_data.push_back(entities.light_data[idx]);
                // note: using u32 ids for now, can use the u64 extension
                pt_lights_ids.push_back(static_cast<u32>(entities.ids[idx]));
            }
        }

        clusters.gl_data.lights_ssbo.update(std::span(pt_light_data.begin(), pt_light_data.end()));
        clusters.gl_data.lights_pos_ssbo.update(std::span(pt_lights_pos.begin(), pt_lights_pos.end()));
        lights_ids_ssbo.update(std::span(pt_lights_ids.begin(), pt_lights_ids.end()));

        pt_light_data.resize(0);
        pt_lights_pos.resize(0);
        pt_lights_ids.resize(0);
    }
 };

void Backend::finish() { ImGui_ImplOpenGL3_Shutdown(); };

} // namespace gl