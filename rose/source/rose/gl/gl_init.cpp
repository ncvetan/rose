#include <rose/gl/gl_init.hpp>

std::optional<rses> init_opengl() {
    if (GLenum glew_success = glewInit(); glew_success != GLEW_OK) {
        return rses().gl(std::format("GLEW failed to initialize: {}", (const char*)glewGetErrorString(glew_success)));
    }

    std::println("GLEW successfully initialized version: {}", (const char*)glewGetString(GLEW_VERSION));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

#ifdef _DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, NULL, GL_FALSE);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
    glDebugMessageCallback(gl_debug_callback, nullptr);
#endif

    return std::nullopt;
}

void GLAPIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei len,
                                  const GLchar* msg, const void* user_param) {
    std::println("GL ERROR: type = {}, severity = {}, message = {}\n", type, severity, msg);
}
