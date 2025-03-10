#ifndef ROSE_INCLUDE_GL_SHADER
#define ROSE_INCLUDE_GL_SHADER

#include <rose/core/alias.hpp>
#include <rose/core/err.hpp>

#include <glm.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>

namespace rose {

struct GL_ShaderCtx {
    fs::path path;
    GLenum type = 0;
};

// wrapper around opengl shaders
struct GL_Shader {
       
    GL_Shader() = default;
    ~GL_Shader();

    std::optional<rses> init(const std::vector<GL_ShaderCtx>& shader_ctxs);
    void use();

    void set_bool(const std::string_view& name, bool value) const;
    void set_int(const std::string_view& name, int value) const;
    void set_float(const std::string_view& name, f32 value) const;
    void set_mat4(const std::string_view& name, const glm::mat4& value) const;
    void set_vec2(const std::string_view& name, const glm::vec2& value) const;
    void set_uvec2(const std::string_view& name, const glm::uvec2& value) const;
    void set_vec3(const std::string_view& name, const glm::vec3&) const;
    void set_uvec3(const std::string_view& name, const glm::uvec3& value) const;
    void set_vec4(const std::string_view& name, const glm::vec4& value) const;

    u32 prg = 0;
};

// all shaders used in the application
struct GL_Shaders {

    std::optional<rses> init();
    
    GL_Shader blur;
    GL_Shader clusters_build;
    GL_Shader clusters_cull;
    GL_Shader gbuf;
    GL_Shader hdr;
    GL_Shader light;
    GL_Shader lighting;
    GL_Shader passthrough;
    GL_Shader dir_shadow;
    GL_Shader pt_shadow;
    GL_Shader skybox;
};

} // namespace rose

#endif
