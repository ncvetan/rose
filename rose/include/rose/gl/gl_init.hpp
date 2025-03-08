#ifndef ROSE_INCLUDE_GL_GL
#define ROSE_INCLUDE_GL_GL

#include <rose/core/err.hpp>

#include <GL/glew.h>

#include <optional>

std::optional<rses> init_opengl();

void GLAPIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei len,
                                  const GLchar* msg, const void* user_param);

#endif