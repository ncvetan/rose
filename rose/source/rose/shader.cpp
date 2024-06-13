#include <rose/logger.hpp>
#include <rose/shader.hpp>

#include <GL/glew.h>
#include <glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace rose {

std::optional<rses> ShaderGL::init(const std::string& vertex_path, const std::string& fragment_path) {
    std::string vertex_code;
    std::string fragment_code;
    std::ifstream vertex_shader_file(vertex_path);
    std::ifstream fragment_shader_file(fragment_path);

    if (!vertex_shader_file) {
        return rses().io("Unable to open vertex shader at path: {}", vertex_path);
    }

    if (!fragment_shader_file) {
        return rses().io("Unable to open fragment shader at path: {}", fragment_path);
    }

    std::stringstream vertex_code_buffer;
    vertex_code_buffer << vertex_shader_file.rdbuf();
    vertex_code = vertex_code_buffer.str();

    std::stringstream fragment_code_buffer;
    fragment_code_buffer << fragment_shader_file.rdbuf();
    fragment_code = fragment_code_buffer.str();

    const char* vertex_shader_code = vertex_code.c_str();
    const char* fragment_shader_code = fragment_code.c_str();

    unsigned int vertex, fragment;
    int success;
    char info_log[512];

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertex_shader_code, NULL);
    glCompileShader(vertex);

    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, info_log);
        return rses().gl("Vertex shader compilation failed at path {}: {}", vertex_path, info_log);
    };

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragment_shader_code, NULL);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, info_log);
        return rses().gl("Fragment shader compilation failed at path {}: {}", fragment_path, info_log);
    };

    prg = glCreateProgram();
    glAttachShader(prg, vertex);
    glAttachShader(prg, fragment);
    glLinkProgram(prg);

    glGetProgramiv(prg, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(prg, 512, NULL, info_log);
        return rses().gl("Shader linking failed: {}", info_log);
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return std::nullopt;
}

ShaderGL::~ShaderGL() {
    if (prg) {
        glDeleteProgram(prg);
    }
}

void ShaderGL::use() { glUseProgram(prg); }

void ShaderGL::set_bool(const std::string& name, bool value) const {
    glProgramUniform1i(prg, glGetUniformLocation(prg, name.c_str()), (int)value);
}
void ShaderGL::set_int(const std::string& name, int value) const {
    glProgramUniform1i(prg, glGetUniformLocation(prg, name.c_str()), value);
}
void ShaderGL::set_float(const std::string& name, float value) const {
    glProgramUniform1f(prg, glGetUniformLocation(prg, name.c_str()), value);
}

void ShaderGL::set_mat4(const std::string& name, const glm::mat4& value) const {
    glProgramUniformMatrix4fv(prg, glGetUniformLocation(prg, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

void ShaderGL::set_vec3(const std::string& name, const glm::vec3& value) const {
    glProgramUniform3f(prg, glGetUniformLocation(prg, name.c_str()), value.x, value.y, value.z);
}

void ShaderGL::set_vec4(const std::string& name, const glm::vec4& value) const {
    glProgramUniform4f(prg, glGetUniformLocation(prg, name.c_str()), value.w, value.x, value.y, value.z);
}

} // namespace rose