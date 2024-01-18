#ifndef ROSE_INCLUDE_SHADER
#define ROSE_INCLUDE_SHADER

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <glm.hpp>

namespace rose {

class ShaderGL {
  public:
    unsigned int id{};

    ShaderGL() = default;
    ~ShaderGL() = default;

    bool init(const std::string& vertex_path, const std::string& fragment_path);
    void use();

    void set_bool(const std::string& name, bool value) const;
    void set_int(const std::string& name, int value) const;
    void set_float(const std::string& name, float value) const;
    void set_mat4(const std::string& name, const glm::mat4& value) const;
    void set_vec3(const std::string& name, const glm::vec3&) const;
    void set_vec4(const std::string& name, const glm::vec4& value) const;
};

} // namespace rose

#endif
