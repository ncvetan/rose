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

struct ShaderInfo {
    fs::path path;
    int type = 0;
};

class ShaderGL {
  public:
    ShaderGL() = default;
    ~ShaderGL();

    std::optional<rses> init(const std::vector<ShaderInfo>& shaders);
    void use();

    void set_bool(const std::string& name, bool value) const;
    void set_int(const std::string& name, int value) const;
    void set_float(const std::string& name, float value) const;
    void set_mat4(const std::string& name, const glm::mat4& value) const;
    void set_vec3(const std::string& name, const glm::vec3&) const;
    void set_vec4(const std::string& name, const glm::vec4& value) const;

    gluint prg = 0;
};

} // namespace rose

#endif
