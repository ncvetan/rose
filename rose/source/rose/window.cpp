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

std::optional<rses> FrameBuf::init(int w, int h) {
    
    glCreateFramebuffers(1, &frame_buf);

    glCreateTextures(GL_TEXTURE_2D, 1, &color_buf);
    glTextureStorage2D(color_buf, 1, GL_RGBA8, w, h);
    glTextureSubImage2D(color_buf, 0, 0, 0, w, h, GL_RGBA16F, GL_FLOAT, nullptr);
    glTextureParameteri(color_buf, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(color_buf, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glCreateRenderbuffers(1, &render_buf);
    glNamedRenderbufferStorage(render_buf, GL_DEPTH_COMPONENT, w, h);

    glNamedFramebufferTexture(frame_buf, GL_COLOR_ATTACHMENT0, color_buf, 0);
    glNamedFramebufferRenderbuffer(frame_buf, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, render_buf);

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

void FrameBuf::draw(ShaderGL& shader) { 
    shader.use();

    glBindVertexArray(vertex_arr);
    glBindTextureUnit(0, color_buf);

    float exposure = 1.0;
    shader.set_float("exposure", exposure);
    shader.set_int("tex", 0);

    glDrawArrays(GL_TRIANGLES, 0, verts.size());
}

FrameBuf::~FrameBuf() {
    glDeleteFramebuffers(1, &frame_buf);
    glDeleteRenderbuffers(1, &render_buf);
    glDeleteTextures(1, &color_buf);
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
        return rses().gl(std::format("Glew failed to initialize: {}", (const char*)glewGetErrorString(glew_success)));
    }

    LOG_INFO("Glew successfully initialized version: {}", (const char*)glewGetString(GLEW_VERSION));

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifdef _DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(err::gl_debug_callback, nullptr);
#endif

    return std::nullopt;
}

std::optional<rses> WindowGLFW::init() {

    std::optional<rses> err = init_glfw();

    if (err) {
        return err.value().general("Unable to initialize GLFW");
    }

    err = init_imgui(window);

    if (err) {
        return err.value().general("Unable to initialize Dear Imgui");
    }

    err = init_opengl();

    if (err) {
        return err.value().general("Unable to initialize GLEW and OpenGL");
    }

    std::optional<rses> es;

    if (es = shaders["skybox"].init({ { SOURCE_DIR "/rose/shaders/gl/skybox.vert", GL_VERTEX_SHADER },
                                      { SOURCE_DIR "/rose/shaders/gl/skybox.frag", GL_FRAGMENT_SHADER } })) {
        err::print(*es);
    }
    if (es = shaders["object"].init({ { SOURCE_DIR "/rose/shaders/gl/object.vert", GL_VERTEX_SHADER },
                                      { SOURCE_DIR "/rose/shaders/gl/object.frag", GL_FRAGMENT_SHADER } })) {
        err::print(*es);
    }
    if (es = shaders["light"].init({ { SOURCE_DIR "/rose/shaders/gl/light.vert", GL_VERTEX_SHADER },
                                     { SOURCE_DIR "/rose/shaders/gl/light.frag", GL_FRAGMENT_SHADER } })) {
        err::print(*es);
    }
    if (es = shaders["shadow"].init({ { SOURCE_DIR "/rose/shaders/gl/shadow.vert", GL_VERTEX_SHADER },
                                      { SOURCE_DIR "/rose/shaders/gl/shadow.frag", GL_FRAGMENT_SHADER },
                                      { SOURCE_DIR "/rose/shaders/gl/shadow.geom", GL_GEOMETRY_SHADER } })) {
        err::print(*es);
    }
    if (es = shaders["hdr"].init({ { SOURCE_DIR "/rose/shaders/gl/hdr.vert", GL_VERTEX_SHADER },
                                      { SOURCE_DIR "/rose/shaders/gl/hdr.frag", GL_FRAGMENT_SHADER }})) {
        err::print(*es);
    }

    // skybox
    world_state.sky_box.init();
    if (es =
            world_state.sky_box.load(texture_manager, { SOURCE_DIR "/assets/skybox/right.jpg", SOURCE_DIR "/assets/skybox/left.jpg",
                                       SOURCE_DIR "/assets/skybox/top.jpg", SOURCE_DIR "/assets/skybox/bottom.jpg",
                                       SOURCE_DIR "/assets/skybox/front.jpg", SOURCE_DIR "/assets/skybox/back.jpg" })) {
        err::print(*es);
    }

    // floor
    tex_cubes.push_back(Object<TexturedCube>({ 0.0f, 0.0f, 0.0f }, { 8.0f, 8.0f, 50.0f }));
    tex_cubes.back().model.init();
    if (es = tex_cubes.back().model.load(texture_manager, SOURCE_DIR "/assets/texture4_diffuse.jpg", SOURCE_DIR "/assets/texture4_spec.jpg", SOURCE_DIR "/assets/texture4_normal.jpg", SOURCE_DIR "/assets/texture4_displace.jpg")) {
        err::print(*es);
    }

    //// cubes
    //tex_cubes.push_back(Object<TexturedCube>({ 2.0f, 1.0f, 5.0f }));
    //tex_cubes.back().model.init();
    //if (es = tex_cubes.back().model.load(texture_manager, SOURCE_DIR "/assets/texture4_diffuse.jpg", SOURCE_DIR "/assets/texture4_spec.jpg", SOURCE_DIR "/assets/texture4_normal.jpg", SOURCE_DIR "/assets/texture4_displace.jpg")) {
    //    err::print(*es);
    //}

    //tex_cubes.push_back(Object<TexturedCube>({ -3.0f, 1.0f, -4.0f }));
    //tex_cubes.back().model.init();
    //if (es = tex_cubes.back().model.load(texture_manager, SOURCE_DIR "/assets/texture4_diffuse.jpg", SOURCE_DIR "/assets/texture4_spec.jpg", SOURCE_DIR "/assets/texture4_normal.jpg", SOURCE_DIR "/assets/texture4_displace.jpg")) {
    //    err::print(*es);
    //}

    // point lights
    pnt_lights.push_back(Object<Cube>({ 0.0f, 0.0f, 19.5f }, { 1.0f, 1.0f, 1.0f }, (u8)ObjectFlags::EMIT_LIGHT));
    pnt_lights.back().model.init();
    pnt_lights.back().light_props.ambient = { 200.0f, 200.0f, 200.0f };
    pnt_lights.back().light_props.diffuse = { 0.3f, 0.3f, 0.3f };
    pnt_lights.back().light_props.specular = { 0.0f, 0.0f, 0.0f };


    if (es = fbuf.init(width, height)) {
        return es;
    }

    // uniform buffer
    glCreateBuffers(1, &world_state.ubo);
    glNamedBufferStorage(world_state.ubo, 208, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, world_state.ubo);

    // omnidirectional shadow map
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

    // shader state that doesn't change between frames
    shaders["shadow"].set_float("far_plane", camera.far_plane);
    shaders["object"].set_float("far_plane", camera.far_plane);

    while (!glfwWindowShouldClose(window)) {

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        dock_id = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        ImGuiIO& io = ImGui::GetIO();
        process_input(window, io.DeltaTime);
        glEnable(GL_DEPTH_TEST);

        // shadow pass ================================================================================================

        glm::mat4 shadow_proj = glm::perspective(glm::radians(90.0f), (float)world_state.shadow.resolution / (float)world_state.shadow.resolution, camera.near_plane, camera.far_plane);
        std::array<glm::mat4, 6> shadow_transforms;

        glViewport(0, 0, world_state.shadow.resolution, world_state.shadow.resolution);
        glBindFramebuffer(GL_FRAMEBUFFER, world_state.shadow.fbo);
        glClear(GL_DEPTH_BUFFER_BIT);
        glCullFace(GL_FRONT); // preventing peter panning

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

            shaders["shadow"].set_mat4("shadow_mats[0]", shadow_transforms[0]);
            shaders["shadow"].set_mat4("shadow_mats[1]", shadow_transforms[1]);
            shaders["shadow"].set_mat4("shadow_mats[2]", shadow_transforms[2]);
            shaders["shadow"].set_mat4("shadow_mats[3]", shadow_transforms[3]);
            shaders["shadow"].set_mat4("shadow_mats[4]", shadow_transforms[4]);
            shaders["shadow"].set_mat4("shadow_mats[5]", shadow_transforms[5]);
            shaders["shadow"].set_vec3("light_pos", light.pos);

            for (auto& cube : tex_cubes) {
                translate(cube.model, cube.pos);
                scale(cube.model, cube.scale);
                cube.draw(shaders["shadow"], world_state);
                cube.model.reset();
            }
        }

        glCullFace(GL_BACK);

        // render pass ================================================================================================
        glBindFramebuffer(GL_FRAMEBUFFER, fbuf.frame_buf);
        glViewport(0, 0, width, height);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        
        glActiveTexture(GL_TEXTURE12);
        glBindTexture(GL_TEXTURE_CUBE_MAP, world_state.shadow.tex);
        shaders["object"].set_int("shadow_map", 12);

        glm::mat4 projection = camera.projection((float)width / (float)height);
        glm::mat4 view = camera.view();

        // update ubo state
        glNamedBufferSubData(world_state.ubo, 0, sizeof(glm::mat4), glm::value_ptr(projection));
        glNamedBufferSubData(world_state.ubo, 128, 16, glm::value_ptr(camera.position));
        glNamedBufferSubData(world_state.ubo, 144, 16, glm::value_ptr(world_state.dir_light.direction));
        glNamedBufferSubData(world_state.ubo, 160, 16, glm::value_ptr(world_state.dir_light.ambient));
        glNamedBufferSubData(world_state.ubo, 176, 16, glm::value_ptr(world_state.dir_light.diffuse));
        glNamedBufferSubData(world_state.ubo, 192, 16, glm::value_ptr(world_state.dir_light.specular));

        // draw calls
        glm::mat4 static_view = view;
        static_view[3] = { 0, 0, 0, 1 }; // removing translation component
        glNamedBufferSubData(world_state.ubo, 64, sizeof(glm::mat4), glm::value_ptr(static_view));
        world_state.sky_box.draw(shaders["skybox"], world_state);
        glNamedBufferSubData(world_state.ubo, 64, sizeof(glm::mat4), glm::value_ptr(view));

        shaders["object"].set_float("gamma_co", gamma);
        shaders["object"].set_float("bias", world_state.shadow.bias);

        for (int i = 0; auto& light : pnt_lights) {
            translate(light.model, light.pos);
            scale(light.model, light.scale);
            light.draw(shaders["light"], world_state);
            light.model.reset();
            // while we render our lights, update our uniforms for use in the object shaders
            shaders["object"].set_vec3(std::format("point_lights[{}].pos", i), light.pos);
            shaders["object"].set_vec3(std::format("point_lights[{}].ambient", i), light.light_props.ambient);
            shaders["object"].set_vec3(std::format("point_lights[{}].diffuse", i), light.light_props.diffuse);
            shaders["object"].set_vec3(std::format("point_lights[{}].specular", i), light.light_props.specular);
            ++i;
        }

        for (auto& cube : tex_cubes) {
            translate(cube.model, cube.pos);
            scale(cube.model, cube.scale);
            cube.draw(shaders["object"], world_state);
            cube.model.reset();
        }

        // render the frame buffer to imgui
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, width, height);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

        fbuf.draw(shaders["hdr"]);

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

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    //WindowGLFW* window_state = (WindowGLFW*)glfwGetWindowUserPointer(window);
    //// handle window resizing
    //glBindTexture(GL_TEXTURE_2D, window_state->fbo_tex->id);
    //glBindFramebuffer(1, window_state->fbo);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, window_state->fbo_tex->id, 0);
    //glBindTexture(GL_TEXTURE_2D, 0);
    //glBindRenderbuffer(GL_RENDERBUFFER, window_state->rbo);
    //glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    //glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, window_state->rbo);
    //glBindRenderbuffer(GL_RENDERBUFFER, 0);
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //glViewport(0, 0, width, height);
}

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
    WindowGLFW* window_state = (WindowGLFW*)glfwGetWindowUserPointer(window);
    window_state->width = width;
    window_state->height = height;
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