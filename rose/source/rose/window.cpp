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

std::optional<rses> FrameBuf::init(int w, int h, const std::vector<FrameBufTexCtx>& texs) {
    
    glCreateFramebuffers(1, &frame_buf);

    tex_bufs.resize(texs.size());
    attachments.resize(texs.size());

    for (int i = 0; i < tex_bufs.size(); ++i) {
        glCreateTextures(GL_TEXTURE_2D, 1, &tex_bufs[i]);
        glTextureStorage2D(tex_bufs[i], 1, texs[i].intern_format, w, h);
        glTextureParameteri(tex_bufs[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(tex_bufs[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(tex_bufs[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(tex_bufs[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glNamedFramebufferTexture(frame_buf, GL_COLOR_ATTACHMENT0 + i, tex_bufs[i], 0);
        attachments[i] = GL_COLOR_ATTACHMENT0 + i;
    }

    // TODO: make having a depth buf optional, unnecessary memory alloc
    glCreateRenderbuffers(1, &render_buf);
    glNamedRenderbufferStorage(render_buf, GL_DEPTH24_STENCIL8, w, h);
    glNamedFramebufferRenderbuffer(frame_buf, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, render_buf);

    if (glCheckNamedFramebufferStatus(frame_buf, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        return rses().gl("framebuffer is incomplete");
    }

    glCreateVertexArrays(1, &vertex_arr);
    glCreateBuffers(1, &vertex_buf);
    glNamedBufferStorage(vertex_buf, verts.size() * sizeof(Vertex), verts.data(), GL_DYNAMIC_STORAGE_BIT);
    glVertexArrayVertexBuffer(vertex_arr, 0, vertex_buf, 0, sizeof(Vertex));

    glEnableVertexArrayAttrib(vertex_arr, 0);
    glEnableVertexArrayAttrib(vertex_arr, 1);

    glVertexArrayAttribFormat(vertex_arr, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, pos));
    glVertexArrayAttribFormat(vertex_arr, 1, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, tex));

    glVertexArrayAttribBinding(vertex_arr, 0, 0);
    glVertexArrayAttribBinding(vertex_arr, 1, 0);

    return std::nullopt;
}

void FrameBuf::draw(ShaderGL& shader, const GlobalState& state) { 
    shader.use();
    glBindVertexArray(vertex_arr);
    glDrawArrays(GL_TRIANGLES, 0, verts.size());
}

FrameBuf::~FrameBuf() {
    glDeleteFramebuffers(1, &frame_buf);
    glDeleteRenderbuffers(1, &render_buf);
    for (auto& buf : tex_bufs) {
        glDeleteTextures(1, &buf);
    }
};

std::optional<rses> WindowGLFW::init_glfw() {
    if (glfwInit() == GLFW_FALSE) {
        return rses().gl("GLFW failed to initialize");
    }

    LOG_INFO("GLFW successfully initialized version: {}", glfwGetVersionString());

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
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
    if (err =
            world_state.sky_box.load(texture_manager, { SOURCE_DIR "/assets/skybox/right.jpg",  SOURCE_DIR "/assets/skybox/left.jpg",
                                                        SOURCE_DIR "/assets/skybox/top.jpg",    SOURCE_DIR "/assets/skybox/bottom.jpg",
                                                        SOURCE_DIR "/assets/skybox/front.jpg",  SOURCE_DIR "/assets/skybox/back.jpg" })) {
        return err;
    }

    tex_cubes.push_back(Object<TexturedCube>({ 0.0f, 0.0f, 0.0f }, { 2.0f, 2.0f, 2.0f }));
    tex_cubes.back().model.init();
    if (err = tex_cubes.back().model.load(
            texture_manager, SOURCE_DIR "/assets/texture4_diffuse.jpg", SOURCE_DIR "/assets/texture4_spec.jpg", 
                             SOURCE_DIR "/assets/texture4_normal.jpg", SOURCE_DIR "/assets/texture4_displace.jpg")) {
        return err;
    }

    tex_cubes.push_back(Object<TexturedCube>({ 0.0f, -5.0f, 0.0f }, { 20.0f, 2.0f, 20.0f }));
    tex_cubes.back().model.init();
    if (err = tex_cubes.back().model.load(
            texture_manager, SOURCE_DIR "/assets/texture4_diffuse.jpg", SOURCE_DIR "/assets/texture4_spec.jpg",
                             SOURCE_DIR "/assets/texture4_normal.jpg", SOURCE_DIR "/assets/texture4_displace.jpg")) {
        return err;
    }

    // models
    objects.push_back(Object<Model>({ 2.0f, 1.0f, 5.0f }));
    if (err = objects.back().model.load(texture_manager, SOURCE_DIR "/assets/model1/model1.obj")) {
        return err;
    }
    for (auto& mesh : objects.back().model.meshes) {
        mesh.init();
    }

    // point lights
    pnt_lights.push_back(Object<Cube>({ 0.0f, 3.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, (u8)ObjectFlags::EMIT_LIGHT));
    pnt_lights.back().model.init();
    pnt_lights.back().light_props.color = { 1.0f, 0.1f, 0.1f, 1.0f };
    pnt_lights.back().light_props.radius();

    pnt_lights.push_back(Object<Cube>({ 0.0f, 3.0f, 2.5f }, { 1.0f, 1.0f, 1.0f }, (u8)ObjectFlags::EMIT_LIGHT));
    pnt_lights.back().model.init();
    pnt_lights.back().light_props.color = { 0.35f, 0.1f, 0.1f, 1.0f };
    pnt_lights.back().light_props.radius();

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

    glCreateBuffers(1, &clusters.clusters_aabb_ssbo);
    glNamedBufferData(clusters.clusters_aabb_ssbo, n_clusters * sizeof(AABB), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, clusters.clusters_aabb_ssbo);

    glCreateBuffers(1, &clusters.light_grid_ssbo);
    glNamedBufferData(clusters.light_grid_ssbo, n_clusters * sizeof(u32) * 2, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, clusters.light_grid_ssbo);

    // intializing to the maximum possible size (each cluster has 100 lights)
    glCreateBuffers(1, &clusters.clusters_indices_ssbo);
    glNamedBufferData(clusters.clusters_indices_ssbo, n_clusters * clusters.max_lights_in_cluster * sizeof(u32),
                      nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, clusters.clusters_indices_ssbo);
    
    glCreateBuffers(1, &clusters.lights_ssbo);
    glCreateBuffers(1, &clusters.lights_pos_ssbo);
   
    // TODO: Get rid of this and come up with a better solution
    update_light_ssbos();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, clusters.lights_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, clusters.lights_pos_ssbo);

    glCreateBuffers(1, &clusters.light_idx_ssbo);
    glNamedBufferData(clusters.light_idx_ssbo, sizeof(u32), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, clusters.light_idx_ssbo);

    std::vector<glm::vec4> clusters_colors(n_clusters);

    for (auto& color : clusters_colors) {
        color.x = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        color.y = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        color.z = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    }

    glCreateBuffers(1, &clusters.clusters_color_ssbo);
    glNamedBufferData(clusters.clusters_color_ssbo, n_clusters * sizeof(glm::vec4), clusters_colors.data(),
                      GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, clusters.clusters_color_ssbo);


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
        glNamedBufferSubData(world_state.ubo, 128, 16, glm::value_ptr(camera.position));
        glNamedBufferSubData(world_state.ubo, 144, 16, glm::value_ptr(world_state.dir_light.direction));
        glNamedBufferSubData(world_state.ubo, 160, 16, glm::value_ptr(world_state.dir_light.color));

        // clustered set-up ===========================================================================================

        // determine the aabb for each cluster
        glm::mat4 inv_proj = glm::inverse(projection);
        shaders.clusters_aabb.use();
        shaders.clusters_aabb.set_mat4("inv_proj", inv_proj);

        glDispatchCompute(16, 9, 24);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // build light lists for each cluster
        shaders.clusters_cull.use();

        glDispatchCompute(n_clusters / 128, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // shadow pass ================================================================================================

        glm::mat4 shadow_proj = glm::perspective(glm::radians(90.0f), (float)world_state.shadow.resolution / (float)world_state.shadow.resolution, camera.near_plane, camera.far_plane);
        std::array<glm::mat4, 6> shadow_transforms;

        // TODO: only call on resize/move
        glViewport(0, 0, world_state.shadow.resolution, world_state.shadow.resolution);
        
        glBindFramebuffer(GL_FRAMEBUFFER, world_state.shadow.fbo);
        glClear(GL_DEPTH_BUFFER_BIT);
        glCullFace(GL_FRONT);           // preventing peter panning

        // TODO: currently only supporting point light shadows, need to take global light into account as well
        for (auto& light : pnt_lights) {
            shadow_transforms[0] = shadow_proj * glm::lookAt(light.pos, light.pos + glm::vec3(1.0f, 0.0f, 0.0f),
                                                             glm::vec3(0.0f, -1.0f, 0.0f));
            shadow_transforms[1] = shadow_proj * glm::lookAt(light.pos, light.pos + glm::vec3(-1.0f, 0.0f, 0.0f),
                                                             glm::vec3(0.0f, -1.0f, 0.0f));
            shadow_transforms[2] = shadow_proj * glm::lookAt(light.pos, light.pos + glm::vec3(0.0f, 1.0f, 0.0f),
                                                             glm::vec3(0.0f, 0.0f, 1.0f));
            shadow_transforms[3] = shadow_proj * glm::lookAt(light.pos, light.pos + glm::vec3(0.0f, -1.0f, 0.0f),
                                                             glm::vec3(0.0f, 0.0f, -1.0f));
            shadow_transforms[4] = shadow_proj * glm::lookAt(light.pos, light.pos + glm::vec3(0.0f, 0.0f, 1.0f),
                                                             glm::vec3(0.0f, -1.0f, 0.0f));
            shadow_transforms[5] = shadow_proj * glm::lookAt(light.pos, light.pos + glm::vec3(0.0f, 0.0f, -1.0f),
                                                             glm::vec3(0.0f, -1.0f, 0.0f));

            shaders.shadow.set_mat4("shadow_mats[0]", shadow_transforms[0]);
            shaders.shadow.set_mat4("shadow_mats[1]", shadow_transforms[1]);
            shaders.shadow.set_mat4("shadow_mats[2]", shadow_transforms[2]);
            shaders.shadow.set_mat4("shadow_mats[3]", shadow_transforms[3]);
            shaders.shadow.set_mat4("shadow_mats[4]", shadow_transforms[4]);
            shaders.shadow.set_mat4("shadow_mats[5]", shadow_transforms[5]);
            shaders.shadow.set_vec3("light_pos", light.pos);

            for (auto& cube : tex_cubes) {
                translate(cube.model, cube.pos);
                scale(cube.model, cube.scale);
                cube.draw(shaders.shadow, world_state);
                cube.model.reset();
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
        glStencilMask(0x00);             // do not write to stencil buffer
        
        glm::mat4 static_view = view;
        static_view[3] = { 0, 0, 0, 1 }; // removing translation component
        glNamedBufferSubData(world_state.ubo, 64, sizeof(glm::mat4), glm::value_ptr(static_view));
        world_state.sky_box.draw(shaders.skybox, world_state);
        glNamedBufferSubData(world_state.ubo, 64, sizeof(glm::mat4), glm::value_ptr(view));
        
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST);

        glActiveTexture(GL_TEXTURE12);  // this texture unit is arbitrary for now
        glBindTexture(GL_TEXTURE_CUBE_MAP, world_state.shadow.tex);
        shaders.lighting.set_int("shadow_map", 12);

        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
        glStencilMask(0xFF);

        for (auto& cube : tex_cubes) {
            translate(cube.model, cube.pos);
            scale(cube.model, cube.scale);
            cube.draw(shaders.gbuf, world_state);
            cube.model.reset();
        }

        for (auto& object : objects) {
            translate(object.model, object.pos);
            scale(object.model, object.scale);
            object.draw(shaders.gbuf, world_state);
            object.model.reset();
        }

        // lighting pass ===========================================================================

        glBindFramebuffer(GL_FRAMEBUFFER, pp1.frame_buf);
        glNamedFramebufferDrawBuffers(pp1.frame_buf, 2, pp1.attachments.data());

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glBlitNamedFramebuffer(gbuf.frame_buf, pp1.frame_buf, 
            0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
        
        glDisable(GL_DEPTH_TEST);
        glStencilFunc(GL_EQUAL, 1, 0xFF);               // compute lighting for all fragments with stencil value '1'
        glStencilOp(GL_ZERO, GL_REPLACE, GL_REPLACE);

        glBindTextureUnit(0, gbuf.tex_bufs[0]);         // positions
        glBindTextureUnit(1, gbuf.tex_bufs[1]);         // normals
        glBindTextureUnit(2, gbuf.tex_bufs[2]);         // colors

        shaders.lighting.set_int("gbuf_pos", 0);
        shaders.lighting.set_int("gbuf_norms", 1);
        shaders.lighting.set_int("gbuf_colors", 2);
        
        pp1.draw(shaders.lighting, world_state);
        
        glStencilFunc(GL_EQUAL, 0, 0xFF);               // pass through for all fragments with stencil value '0'
        glStencilOp(GL_REPLACE, GL_ZERO, GL_ZERO);
        shaders.passthrough.set_int("gbuf_colors", 2);
        pp1.draw(shaders.passthrough, world_state);

        // forward rendered =======================================================================
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);

        for (auto& light : pnt_lights) {
            translate(light.model, light.pos);
            scale(light.model, light.scale);
            light.draw(shaders.light, world_state);
            light.model.reset();
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
                } 
                else {
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

        glBindTextureUnit(0, pp1.tex_bufs[0]);  // color
        glBindTextureUnit(1, pp1.tex_bufs[1]);  // blur

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

void WindowGLFW::enable_vsync (bool enable) { (enable) ? glfwSwapInterval(1) : glfwSwapInterval(0); };

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