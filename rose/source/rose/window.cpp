#include <rose/err.hpp>
#include <rose/gui.hpp>
#include <rose/logger.hpp>
#include <rose/model.hpp>
#include <rose/window.hpp>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <stb_image.h>

#include <array>
#include <format>
#include <iostream>

namespace rose {

std::optional<rses> WindowGLFW::init_glfw() {
    if (glfwInit() == GLFW_FALSE) {
        return rses().gl("GLFW failed to initialize");
    }

    LOG_INFO("GLFW successfully initialized version: {}", glfwGetVersionString());

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

#ifdef _DEBUG
    glfwWindowHint(GLFW_CONTEXT_DEBUG, GL_TRUE);
#endif

    window = glfwCreateWindow(width, height, name.c_str(), NULL, NULL);

    if (!window) {
        glfwTerminate();
        return rses().gl("Failed to create GLFW window");
    }

    LOG_INFO("GLFW window has been successfully created");

    glfwSetWindowUserPointer(window, this);
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeCallback(window, resize_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    return std::nullopt;
}

std::optional<rses> WindowGLFW::init_imgui(GLFWwindow* window) {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    return std::nullopt;
}

std::optional<rses> WindowGLFW::init_opengl() {
    if (GLenum glew_success = glewInit(); glew_success != GLEW_OK) {
        return rses().gl(std::format("GLEW failed to initialize: {}", (const char*)glewGetErrorString(glew_success)));
    }

    LOG_INFO("Glew successfully initialized version: {}", (const char*)glewGetString(GLEW_VERSION));

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

#ifdef _DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, NULL, GL_FALSE);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
    glDebugMessageCallback(err::gl_debug_callback, nullptr);
#endif

    return std::nullopt;
}

std::optional<rses> WindowGLFW::init() {

    std::optional<rses> err = std::nullopt;

    if (err = init_glfw()) {
        return err.value().general("Unable to initialize GLFW");
    }

    if (err = init_imgui(window)) {
        return err.value().general("Unable to initialize Dear ImGui");
    }

    if (err = init_opengl()) {
        return err.value().general("Unable to initialize GLEW and OpenGL");
    }

    // shader initialization ======================================================================

    if (err = shaders.init()) {
        return err.value().general("unable to initialize shaders");
    }

    // object initialization ======================================================================
    world_state.sky_box.init();
    if (err = world_state.sky_box.load(
            texture_manager, { SOURCE_DIR "/assets/skybox/right.jpg", SOURCE_DIR "/assets/skybox/left.jpg",
                               SOURCE_DIR "/assets/skybox/top.jpg", SOURCE_DIR "/assets/skybox/bottom.jpg",
                               SOURCE_DIR "/assets/skybox/front.jpg", SOURCE_DIR "/assets/skybox/back.jpg" })) {
        return err;
    }

    // models
    ObjectCtx sponza_def = { .model_pth = SOURCE_DIR "/assets/Sponza/glTF/Sponza.gltf",
                        .pos = { 0.0f, 0.0f, 0.0f },
                        .scale = { 0.02f, 0.02f, 0.02f },
                        .light_props = PointLight(),
                        .flags = ObjectFlags::NONE };

    objects.add_object(texture_manager, sponza_def);
    
    // TODO: Fill in light path...
    ObjectCtx light1_def = { .model_pth = SOURCE_DIR "/assets/model1/model1.obj",
                             .pos = { 0.0f, 3.0f, 0.0f },
                             .scale = { 1.0f, 1.0f, 1.0f },
                             .light_props = PointLight(),
                             .flags = ObjectFlags::EMIT_LIGHT 
                           };

    objects.add_object(texture_manager, light1_def);
    objects.light_props.back().radius(world_state.exposure);
    objects.light_props.back().color = { 1.0f, 0.1f, 0.1f, 1.0f };

    ObjectCtx light2_def = { .model_pth = SOURCE_DIR "/assets/model1/model1.obj",
                             .pos = { 0.0f, 3.0f, 2.5f },
                             .scale = { 1.0f, 1.0f, 1.0f },
                             .light_props = PointLight(),
                             .flags = ObjectFlags::EMIT_LIGHT };

    objects.add_object(texture_manager, light2_def);
    objects.light_props.back().radius(world_state.exposure);
    objects.light_props.back().color = { 0.35f, 0.1f, 0.1f, 1.0f };

    world_state.dir_light.color = { 0.7f, 0.7f, 0.7f };

    // frame buf initialization ===================================================================
    if (err = gbuf.init(width, height,
                        { { GL_RGBA16F, GL_RGBA, GL_FLOAT },
                          { GL_RGBA16F, GL_RGBA, GL_FLOAT },
                          { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE } })) {
        return err;
    }

    if (err = pp1.init(width, height, { { GL_RGBA16F, GL_RGBA, GL_FLOAT }, { GL_RGBA16F, GL_RGBA, GL_FLOAT } })) {
        return err;
    }

    if (err = pp2.init(width, height, { { GL_RGBA16F, GL_RGBA, GL_FLOAT } })) {
        return err;
    }

    if (err = fbuf_out.init(width, height, { { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE } })) {
        return err;
    }

    // uniform buffer initialization ==============================================================

    glCreateBuffers(1, &world_state.ubo);
    glNamedBufferStorage(world_state.ubo, 208, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, world_state.ubo);

    glm::uvec2 screen_dims = { width, height };
    glNamedBufferSubData(world_state.ubo, 176, 16, glm::value_ptr(clusters.grid_sz));
    glNamedBufferSubData(world_state.ubo, 192, 8, glm::value_ptr(screen_dims));
    glNamedBufferSubData(world_state.ubo, 200, 4, &camera.far_plane);
    glNamedBufferSubData(world_state.ubo, 204, 4, &camera.near_plane);

    // ssbo initialization ========================================================================

    s32 n_clusters = clusters.grid_sz.x * clusters.grid_sz.y * clusters.grid_sz.z;

    clusters.clusters_aabb_ssbo.init(sizeof(AABB), n_clusters, 2);
    // initially allocate memory for 1024 point lights
    clusters.lights_ssbo.init(sizeof(PointLight), 1024, 3);
    clusters.lights_pos_ssbo.init(sizeof(glm::vec4), 1024, 4);
    update_light_ssbos();
    clusters.clusters_ssbo.init(sizeof(u32) * (1 + clusters.max_lights_in_cluster), n_clusters, 5);

    // shadow map initialization ==================================================================
    glGenFramebuffers(1, &world_state.shadow.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, world_state.shadow.fbo);
    glGenTextures(1, &world_state.shadow.tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, world_state.shadow.tex);

    for (int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, world_state.shadow.resolution,
                     world_state.shadow.resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, world_state.shadow.tex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return std::nullopt;
};

void WindowGLFW::update() {
    while (!glfwWindowShouldClose(window)) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        dock_id = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        ImGuiIO& io = ImGui::GetIO();
        process_input(window, io.DeltaTime);
        glEnable(GL_DEPTH_TEST);

        glm::mat4 projection = camera.projection((float)width / (float)height);
        glm::mat4 view = camera.view();
        s32 n_clusters = clusters.grid_sz.x * clusters.grid_sz.y * clusters.grid_sz.z;

        // update ubo state
        glNamedBufferSubData(world_state.ubo, 0, 64, glm::value_ptr(projection));
        glNamedBufferSubData(world_state.ubo, 64, 64, glm::value_ptr(view));
        glNamedBufferSubData(world_state.ubo, 128, 16, glm::value_ptr(camera.position));
        glNamedBufferSubData(world_state.ubo, 144, 16, glm::value_ptr(world_state.dir_light.direction));
        glNamedBufferSubData(world_state.ubo, 160, 16, glm::value_ptr(world_state.dir_light.color));

        // clustered set-up ===========================================================================================

        // determine the aabb for each cluster
        // note: this only needs to be called once if parameters do not change
        shaders.clusters_build.use();
        shaders.clusters_build.set_mat4("inv_proj", glm::inverse(projection));

        glDispatchCompute(clusters.grid_sz.x, clusters.grid_sz.y, clusters.grid_sz.z);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // build light lists for each cluster
        shaders.clusters_cull.use();

        glDispatchCompute(27, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // shadow pass ================================================================================================

        glm::mat4 shadow_proj = glm::perspective(
            glm::radians(90.0f), (float)world_state.shadow.resolution / (float)world_state.shadow.resolution,
            camera.near_plane, camera.far_plane);
        std::array<glm::mat4, 6> shadow_transforms;

        glBindFramebuffer(GL_FRAMEBUFFER, world_state.shadow.fbo);
        glViewport(0, 0, world_state.shadow.resolution, world_state.shadow.resolution);
        glClear(GL_DEPTH_BUFFER_BIT);
        glCullFace(GL_FRONT); // preventing peter panning

        // TODO: currently only supporting point light shadows, need to take global light into account as well
        for (size_t light_idx = 0; light_idx < objects.size(); ++light_idx) {
            
            if (!(objects.flags[light_idx] & ObjectFlags::EMIT_LIGHT)) {
                continue;
            }

            glm::vec3 light_pos = objects.posns[light_idx];
            
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

            shaders.shadow.set_mat4("shadow_mats[0]", shadow_transforms[0]);
            shaders.shadow.set_mat4("shadow_mats[1]", shadow_transforms[1]);
            shaders.shadow.set_mat4("shadow_mats[2]", shadow_transforms[2]);
            shaders.shadow.set_mat4("shadow_mats[3]", shadow_transforms[3]);
            shaders.shadow.set_mat4("shadow_mats[4]", shadow_transforms[4]);
            shaders.shadow.set_mat4("shadow_mats[5]", shadow_transforms[5]);
            shaders.shadow.set_vec3("light_pos", light_pos);

            for (size_t obj_idx = 0; obj_idx < objects.size(); ++obj_idx) {
                if (!(objects.flags[obj_idx] & ObjectFlags::EMIT_LIGHT)) {
                    translate(objects.models[obj_idx], objects.posns[obj_idx]);
                    scale(objects.models[obj_idx], objects.scales[obj_idx]);
                    objects.models[obj_idx].draw(shaders.shadow, world_state);
                    objects.models[obj_idx].reset();
                }
            }
        }

        glCullFace(GL_BACK);

        // geometry pass ==========================================================================
        glBindFramebuffer(GL_FRAMEBUFFER, gbuf.frame_buf);
        glNamedFramebufferDrawBuffers(gbuf.frame_buf, 3, gbuf.attachments.data());

        glViewport(0, 0, width, height);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // draw skybox
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glStencilMask(0x00); // do not write to stencil buffer

        glm::mat4 static_view = view;
        static_view[3] = { 0, 0, 0, 1 }; // removing translation component
        glNamedBufferSubData(world_state.ubo, 64, sizeof(glm::mat4), glm::value_ptr(static_view));
        world_state.sky_box.draw(shaders.skybox, world_state);
        glNamedBufferSubData(world_state.ubo, 64, sizeof(glm::mat4), glm::value_ptr(view));

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST);

        glActiveTexture(GL_TEXTURE12); // this texture unit is arbitrary for now
        glBindTexture(GL_TEXTURE_CUBE_MAP, world_state.shadow.tex);
        shaders.lighting.set_int("shadow_map", 12);

        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
        glStencilMask(0xFF);

        for (size_t idx = 0; idx < objects.size(); ++idx) {
            if (!(objects.flags[idx] & ObjectFlags::EMIT_LIGHT)) {                
                translate(objects.models[idx], objects.posns[idx]);
                scale(objects.models[idx], objects.scales[idx]);
                objects.models[idx].draw(shaders.gbuf, world_state);
                objects.models[idx].reset();
            }
        }

        // lighting pass ===========================================================================

        glBindFramebuffer(GL_FRAMEBUFFER, pp1.frame_buf);
        glNamedFramebufferDrawBuffers(pp1.frame_buf, 2, pp1.attachments.data());

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glBlitNamedFramebuffer(gbuf.frame_buf, pp1.frame_buf, 0, 0, width, height, 0, 0, width, height,
                               GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

        glDisable(GL_DEPTH_TEST);
        glStencilFunc(GL_EQUAL, 1, 0xFF); // compute lighting for all fragments with stencil value '1'
        glStencilOp(GL_ZERO, GL_REPLACE, GL_REPLACE);

        glBindTextureUnit(0, gbuf.tex_bufs[0]); // positions (ws)
        glBindTextureUnit(1, gbuf.tex_bufs[1]); // normals
        glBindTextureUnit(2, gbuf.tex_bufs[2]); // colors

        shaders.lighting.set_int("gbuf_pos", 0);
        shaders.lighting.set_int("gbuf_norms", 1);
        shaders.lighting.set_int("gbuf_colors", 2);

        pp1.draw(shaders.lighting, world_state);

        glStencilFunc(GL_EQUAL, 0, 0xFF); // pass through for all fragments with stencil value '0'
        glStencilOp(GL_REPLACE, GL_ZERO, GL_ZERO);
        shaders.passthrough.set_int("gbuf_colors", 2);
        pp1.draw(shaders.passthrough, world_state);

        // forward rendered =======================================================================
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);

        // draw light emitters
        for (size_t idx = 0; idx < objects.size(); ++idx) {
            if (objects.flags[idx] & ObjectFlags::EMIT_LIGHT) {
                translate(objects.models[idx], objects.posns[idx]);
                scale(objects.models[idx], objects.scales[idx]);
                objects.models[idx].draw(shaders.light, world_state);
                objects.models[idx].reset();
            }
        }

        // post processing passes =================================================================

        // compute bloom
        if (world_state.bloom) {
            glBindFramebuffer(GL_FRAMEBUFFER, pp1.frame_buf);
            glNamedFramebufferDrawBuffer(pp1.frame_buf, GL_COLOR_ATTACHMENT1);
            glClear(GL_DEPTH_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);
            bool horizontal = false;
            glBindTextureUnit(0, pp1.tex_bufs[1]); // brightness buf
            shaders.blur.set_int("tex", 0);
            pp1.draw(shaders.blur, world_state);

            for (int i = 0; i < world_state.n_bloom_passes * 2 - 1; ++i) {
                if (horizontal) {
                    glBindFramebuffer(GL_FRAMEBUFFER, pp1.frame_buf);
                    glNamedFramebufferDrawBuffer(pp1.frame_buf, GL_COLOR_ATTACHMENT1);
                    glClear(GL_DEPTH_BUFFER_BIT);
                    glDisable(GL_DEPTH_TEST);
                    glBindTextureUnit(0, pp2.tex_bufs[0]);
                    shaders.blur.set_bool("horizontal", true);
                    shaders.blur.set_int("tex", 0);
                    pp1.draw(shaders.blur, world_state);
                } else {
                    glBindFramebuffer(GL_FRAMEBUFFER, pp2.frame_buf);
                    glClear(GL_DEPTH_BUFFER_BIT);
                    glDisable(GL_DEPTH_TEST);
                    glBindTextureUnit(0, pp1.tex_bufs[1]);
                    shaders.blur.set_bool("horizontal", false);
                    shaders.blur.set_int("tex", 0);
                    pp2.draw(shaders.blur, world_state);
                }
                horizontal = !horizontal;
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, fbuf_out.frame_buf);
        glClear(GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

        glBindTextureUnit(0, pp1.tex_bufs[0]); // color
        glBindTextureUnit(1, pp1.tex_bufs[1]); // blur

        shaders.hdr.set_int("scene_tex", 0);
        shaders.hdr.set_int("blur_tex", 1);
        shaders.hdr.set_float("gamma", world_state.gamma);
        shaders.hdr.set_float("exposure", world_state.exposure);
        shaders.hdr.set_bool("bloom_enabled", world_state.bloom);
        fbuf_out.draw(shaders.hdr, world_state);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        gui::imgui(*this);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
};

void WindowGLFW::destroy() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    window = nullptr;
};

void WindowGLFW::enable_vsync(bool enable) { (enable) ? glfwSwapInterval(1) : glfwSwapInterval(0); };

void framebuffer_size_callback(GLFWwindow* window, int width, int height) { return; }

void mouse_callback(GLFWwindow* window, double xpos_in, double ypos_in) {
    WindowGLFW* window_state = (WindowGLFW*)glfwGetWindowUserPointer(window);
    window_state->mouse_xy = { (float)xpos_in, (float)ypos_in };
    if (window_state->glfw_captured) {
        float xpos = static_cast<float>(xpos_in);
        float ypos = static_cast<float>(ypos_in);
        float xoffset = xpos - window_state->last_xy.x;
        float yoffset = window_state->last_xy.y - ypos;
        window_state->last_xy.x = xpos;
        window_state->last_xy.y = ypos;
        window_state->camera.process_mouse_movement(xoffset, yoffset);
    }
}

void resize_callback(GLFWwindow* window, int width, int height) {
    // TODO: implement necessary resize state changes
    WindowGLFW* window_state = (WindowGLFW*)glfwGetWindowUserPointer(window);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    WindowGLFW* window_state = (WindowGLFW*)glfwGetWindowUserPointer(window);
    if (window_state->glfw_captured) {
        window_state->camera.process_mouse_scroll(static_cast<float>(yoffset));
    }
}

void process_input(GLFWwindow* window, float delta_time) {
    WindowGLFW* window_state = (WindowGLFW*)glfwGetWindowUserPointer(window);
    if (window_state->glfw_captured) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            window_state->camera.process_keyboard(CameraMovement::FORWARD, delta_time);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            window_state->camera.process_keyboard(CameraMovement::BACKWARD, delta_time);
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            window_state->camera.process_keyboard(CameraMovement::LEFT, delta_time);
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            window_state->camera.process_keyboard(CameraMovement::RIGHT, delta_time);
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            window_state->camera.process_keyboard(CameraMovement::DOWN, delta_time);
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            window_state->camera.process_keyboard(CameraMovement::UP, delta_time);
        }
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (window_state->vp_focused && window_state->vp_rect.contains(window_state->mouse_xy)) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            window_state->glfw_captured = true;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        window_state->glfw_captured = false;
    }
}
} // namespace rose