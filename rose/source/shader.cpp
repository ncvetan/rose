#include <GL/glew.h>
#include <glm.hpp>

#include <logger.hpp>
#include <shader.hpp>

namespace rose {

bool ShaderGL::init(const std::string& vertex_path, const std::string& fragment_path) {
    std::string vertex_code{};
    std::string fragment_code{};
    std::ifstream vertex_shader_file(vertex_path);
    std::ifstream fragment_shader_file(fragment_path);

    if (!vertex_shader_file) {
        LOG_ERROR("Unable to open vertex shader at path: {}", vertex_path);
        return false;
    }

    if (!fragment_shader_file) {
        LOG_ERROR("Unable to open fragment shader at path: {}", fragment_path);
        return false;
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
        LOG_ERROR("Vertex shader compilation failed: {}", info_log);
        return false;
    };

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragment_shader_code, NULL);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, info_log);
        LOG_ERROR("Fragment shader compilation failed: {}", info_log);
        return false;
    };

    id = glCreateProgram();
    glAttachShader(id, vertex);
    glAttachShader(id, fragment);
    glLinkProgram(id);

    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(id, 512, NULL, info_log);
        LOG_ERROR("Shader linking failed: {}", info_log);
        return false;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return true;
}

void ShaderGL::use() { glUseProgram(id); }

void ShaderGL::set_bool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(id, name.c_str()), (int)value);
}
void ShaderGL::set_int(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(id, name.c_str()), value);
}
void ShaderGL::set_float(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(id, name.c_str()), value);
}

void ShaderGL::set_mat4(const std::string& name, const glm::mat4& value) const {
    glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, &value[0][0]);
}

} // namespace rose