// =============================================================================
//   abstractions over OpenGL shaders
// =============================================================================

#ifndef ROSE_INCLUDE_BACKENDS_GL_SHADER
#define ROSE_INCLUDE_BACKENDS_GL_SHADER

#include <rose/core/core.hpp>
#include <rose/core/err.hpp>

#include <glm.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>

namespace gl {

struct ShaderCtx {
    fs::path path;
    GLenum type = 0;
};

// wrapper around opengl shaders
struct Shader {

    Shader() = default;
    ~Shader();

    rses init(const std::vector<ShaderCtx>& shader_ctxs);
    void use();

    void set_bool(const std::string_view& name, bool value) const;
    void set_u32(const std::string_view& name, u32 value) const;
    void set_i32(const std::string_view& name, int value) const;
    void set_tex(const std::string_view& name, int value, u32 tex) const;
    void set_f32(const std::string_view& name, f32 value) const;
    void set_mat4(const std::string_view& name, const glm::mat4& value) const;
    void set_vec2(const std::string_view& name, const glm::vec2& value) const;
    void set_uvec2(const std::string_view& name, const glm::uvec2& value) const;
    void set_vec3(const std::string_view& name, const glm::vec3&) const;
    void set_uvec3(const std::string_view& name, const glm::uvec3& value) const;
    void set_vec4(const std::string_view& name, const glm::vec4& value) const;

    u32 prg = 0;
};

// all shaders used in the application
struct Shaders {

    rses init();

    Shader bloom;
    Shader brightness;
    Shader clusters_build;
    Shader clusters_cull;
    Shader gbuf;
    Shader out;
    Shader light;
    Shader lighting_deferred;
    Shader lighting_forward;
    Shader passthrough;
    Shader dir_shadow;
    Shader pt_shadow;
    Shader skybox;
};

}

#endif