#include <rose/logger.hpp>
#include <rose/shader.hpp>

#include <GL/glew.h>
#include <glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <expected>

namespace rose {

static std::expected<GLuint, rses> create_vertex_shader(const std::string& vertex_path) { 
    std::string vertex_code; 
    std::ifstream vertex_shader_file(vertex_path);
    
    if (!vertex_shader_file) {
        return std::unexpected(rses().io("Unable to open vertex shader at path: {}", vertex_path));
    }
    
    std::stringstream vertex_code_buffer;
    vertex_code_buffer << vertex_shader_file.rdbuf();
    vertex_code = vertex_code_buffer.str();
    const char* vertex_shader_code = vertex_code.c_str();
    GLuint vertex;
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertex_shader_code, NULL);
    glCompileShader(vertex);
    int success = 0;
    char info_log[512];
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, info_log);
        return std::unexpected(rses().gl("Vertex shader compilation failed at path {}: {}", vertex_path, info_log));
    };

    return vertex;
}

static std::expected<GLuint, rses> create_fragment_shader(const std::string& fragment_path) {
    std::string fragment_code;
    std::ifstream fragment_shader_file(fragment_path);
    
    if (!fragment_shader_file) {
        return std::unexpected(rses().io("Unable to open fragment shader at path: {}", fragment_path));
    }
    
    std::stringstream fragment_code_buffer;
    fragment_code_buffer << fragment_shader_file.rdbuf();
    fragment_code = fragment_code_buffer.str();
    const char* fragment_shader_code = fragment_code.c_str();
    GLuint fragment;
    int success = 0;
    char info_log[512];

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragment_shader_code, NULL);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, info_log);
        return std::unexpected(rses().gl("Fragment shader compilation failed at path {}: {}", fragment_path, info_log));
    };

    return fragment;
}

static std::expected<GLuint, rses> create_geometry_shader(const std::string& geometry_path) {
    std::string geometry_code;
    std::ifstream geometry_shader_file(geometry_path);

    if (!geometry_shader_file) {
        return std::unexpected(rses().io("Unable to open geometry shader at path: {}", geometry_path));
    }

    std::stringstream geometry_code_buffer;
    geometry_code_buffer << geometry_shader_file.rdbuf();
    geometry_code = geometry_code_buffer.str();
    const char* geometry_shader_code = geometry_code.c_str();
    GLuint geometry;
    int success = 0;
    char info_log[512];

    geometry = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(geometry, 1, &geometry_shader_code, NULL);
    glCompileShader(geometry);

    glGetShaderiv(geometry, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(geometry, 512, NULL, info_log);
        return std::unexpected(rses().gl("Geometry shader compilation failed at path {}: {}", geometry_path, info_log));
    };

    return geometry;
}

std::optional<rses> ShaderGL::init(const std::string& vertex_path, const std::string& fragment_path) {
    std::expected<GLuint, rses> vertex = create_vertex_shader(vertex_path);
    if (!vertex.has_value()) {
        return vertex.error();
    }

    std::expected<GLuint, rses> fragment = create_fragment_shader(fragment_path);
    if (!fragment.has_value()) {
        return fragment.error();
    }

    prg = glCreateProgram();
    glAttachShader(prg, *vertex);
    glAttachShader(prg, *fragment);
    glLinkProgram(prg);

    int success = 0;
    char info_log[512];

    glGetProgramiv(prg, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(prg, 512, NULL, info_log);
        return rses().gl("Shader linking failed: {}", info_log);
    }

    glDeleteShader(*vertex);
    glDeleteShader(*fragment);
    return std::nullopt;
}

std::optional<rses> ShaderGL::init(const std::string& vertex_path, const std::string& fragment_path, const std::string& geometry_path) {
    std::expected<GLuint, rses> vertex = create_vertex_shader(vertex_path);
    if (!vertex.has_value()) {
        return vertex.error();
    }

    std::expected<GLuint, rses> fragment = create_fragment_shader(fragment_path);
    if (!fragment.has_value()) {
        return fragment.error();
    }

    std::expected<GLuint, rses> geometry = create_geometry_shader(geometry_path);
    if (!geometry.has_value()) {
        return geometry.error();
    }

    prg = glCreateProgram();
    glAttachShader(prg, *vertex);
    glAttachShader(prg, *fragment);
    glAttachShader(prg, *geometry);
    glLinkProgram(prg);

    int success = 0;
    char info_log[512];

    glGetProgramiv(prg, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(prg, 512, NULL, info_log);
        return rses().gl("Shader linking failed: {}", info_log);
    }

    glDeleteShader(*vertex);
    glDeleteShader(*fragment);
    glDeleteShader(*geometry);
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