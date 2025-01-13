#ifndef ROSE_INCLUDE_SHADER
#define ROSE_INCLUDE_SHADER

#include <rose/alias.hpp>
#include <rose/err.hpp>

#include <glm.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace rose {

struct ShaderCtx {
    fs::path path;
    GLenum type = 0;
};

// wrapper around opengl shaders
struct ShaderGL {
       
    ShaderGL() = default;
    ~ShaderGL();

    std::optional<rses> init(const std::vector<ShaderCtx>& shader_ctxs);
    void use();

    void set_bool(const std::string& name, bool value) const;
    void set_int(const std::string& name, int value) const;
    void set_float(const std::string& name, float value) const;
    void set_mat4(const std::string& name, const glm::mat4& value) const;
    void set_vec2(const std::string& name, const glm::vec2& value) const;
    void set_uvec2(const std::string& name, const glm::uvec2& value) const;
    void set_vec3(const std::string& name, const glm::vec3&) const;
    void set_uvec3(const std::string& name, const glm::uvec3& value) const;
    void set_vec4(const std::string& name, const glm::vec4& value) const;

    gluint prg = 0;
};

// all shaders used in the application
struct ShadersGL {

    std::optional<rses> init();
    
    ShaderGL blur;
    ShaderGL clusters_build;
    ShaderGL clusters_cull;
    ShaderGL gbuf;
    ShaderGL hdr;
    ShaderGL light;
    ShaderGL lighting;
    ShaderGL passthrough;
    ShaderGL shadow;
    ShaderGL skybox;
};

} // namespace rose

#endif
