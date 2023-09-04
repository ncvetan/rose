#include <GL/glew.h>
#include <glm.hpp>

#include <logger.hpp>
#include <shader.hpp>

bool ShaderGL::init(const std::string& vertex_path, const std::string& fragment_path)
{
    bool init_success = true;
    std::string vertex_code;
    std::string fragment_code;
    std::ifstream vertex_shader_file;
    std::ifstream fragment_shader_file;

    vertex_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fragment_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        vertex_shader_file.open(vertex_path);
        fragment_shader_file.open(fragment_path);
        std::stringstream vShaderStream, fShaderStream;

        vShaderStream << vertex_shader_file.rdbuf();
        fShaderStream << fragment_shader_file.rdbuf();

        vertex_shader_file.close();
        fragment_shader_file.close();

        vertex_code = vShaderStream.str();
        fragment_code = fShaderStream.str();
    }
    catch (std::ifstream::failure e)
    {
        LOG_ERROR("Shader could not be successfully read!");
        init_success = false;
    }

    const char* vertex_shader_code = vertex_code.c_str();
    const char* fragment_shader_code = fragment_code.c_str();

    unsigned int vertex, fragment;
    int success;
    char info_log[512];

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertex_shader_code, NULL);
    glCompileShader(vertex);

    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex, 512, NULL, info_log);
        LOG_ERROR("Vertex shader compilation failed: {}", info_log);
        init_success = false;
    };

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragment_shader_code, NULL);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragment, 512, NULL, info_log);
        LOG_ERROR("Fragment shader compilation failed: {}", info_log);
        init_success = false;
    };

    id = glCreateProgram();
    glAttachShader(id, vertex);
    glAttachShader(id, fragment);
    glLinkProgram(id);

    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(id, 512, NULL, info_log);
        LOG_ERROR("Shader linking failed: {}", info_log);
        init_success = false;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return init_success;
}

ShaderGL::~ShaderGL() { }

void ShaderGL::use() { glUseProgram(id); }

void ShaderGL::set_bool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(id, name.c_str()), (int)value);
}
void ShaderGL::set_int(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(id, name.c_str()), value);
}
void ShaderGL::set_float(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(id, name.c_str()), value);
}

void ShaderGL::set_mat4(const std::string& name, const glm::mat4& value) const
{
    glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, &value[0][0]);
}