#include <rose/err.hpp>

#include <GL/glew.h>

#include <cassert>
#include <iostream>
#include <sstream>

namespace err {
    
void GLAPIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei len, const GLchar* msg,
                                 const void* user_param) {

  std::cout << std::format("GL ERROR: type = {}, severity = {}, message = {}\n", type, severity, msg);
}

rses Handle::no_memory() {
    Error err;
    err.ty = Error::Type::NO_MEMORY;
    err.msg = "Out of memory";
    return add_error(std::move(err));
}

rses Handle::general(std::string&& errmsg) {
    Error err;
    err.ty = Error::Type::GENERAL;
    err.msg = std::move(errmsg);
    return add_error(std::move(err));
}

rses Handle::core(std::string&& errmsg) {
    Error err;
    err.ty = Error::Type::CORE;
    err.msg = std::move(errmsg);
    return add_error(std::move(err));
}

rses Handle::io(std::string&& errmsg) {
    Error err;
    err.ty = Error::Type::IO;
    err.msg = std::move(errmsg);
    return add_error(std::move(err));
}

rses Handle::gl(std::string&& errmsg) {
    Error err;
    err.msg = std::move(errmsg);
    err.ty = Error::Type::GL;
    switch (glGetError()) {
    case GL_INVALID_ENUM:
        new (&err.gl_err_msg) std::string("INVALID_ENUM");
        break;
    case GL_INVALID_VALUE:
        new (&err.gl_err_msg) std::string("INVALID_VALUE");
        break;
    case GL_INVALID_OPERATION:
        new (&err.gl_err_msg) std::string("INVALID_OPERATION");
        break;
    case GL_STACK_OVERFLOW:
        new (&err.gl_err_msg) std::string("STACK_OVERFLOW");
        break;
    case GL_STACK_UNDERFLOW:
        new (&err.gl_err_msg) std::string("STACK_UNDERFLOW");
        break;
    case GL_OUT_OF_MEMORY:
        new (&err.gl_err_msg) std::string("OUT_OF_MEMORY");
        break;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        new (&err.gl_err_msg) std::string("INVALID_FRAMEBUFFER_OPERATION");
        break;
    default:
        new (&err.gl_err_msg) std::string("ERR");
        break;
    }
    return add_error(std::move(err));
}

rses Handle::error_code(const std::error_code& ec) {
    Error err;
    err.ty = Error::Type::ERROR_CODE;
    new (&err.ec) std::error_code(ec);
    return add_error(std::move(err));
}

rses Handle::add_error(Error&& err) {
    err_stack.push_back(std::move(err));
    return *this;
}

std::string Error::str() const {
    std::stringstream ss;
    switch (ty) {
    case Type::INVALID:
        ss << "<<Error enum invalid>>";
        break;
    case Type::NO_MEMORY:
        ss << "Memory error:\n    " << msg;
        break;
    case Type::GENERAL:
        ss << "General error:\n    " << msg;
        break;
    case Type::CORE:
        ss << "Core error:\n    " << msg;
        break;
    case Type::IO:
        ss << "IO error:\n    " << msg;
        break;
    case Type::GL:
        ss << "OpenGL error:\n    " << msg << " (" << gl_err_msg << ")";
        break;
    case Type::ERROR_CODE:
        ss << "C++ error:\n    " << ec.message();
        break;
    default:
        assert(false);
        ss << "<<Error enum out of bounds>>";
        break;
    }
    return ss.str();
}

Error::Error(const Error& err) {
    ty = err.ty;
    msg = err.msg;

    switch (ty) {
    case Type::INVALID:
        break;
    case Type::NO_MEMORY:
        break;
    case Type::GENERAL:
        break;
    case Type::CORE:
        break;
    case Type::IO:
        break;
    case Type::GL:
        new (&gl_err_msg) std::string(err.gl_err_msg);
        break;
    case Type::ERROR_CODE:
        new (&ec) std::error_code(err.ec);
        break;
    default:
        assert(false);
    }
}

Error::Error(Error&& err) noexcept {
    ty = err.ty;
    msg = std::move(err.msg);

    switch (ty) {
    case Type::INVALID:
        break;
    case Type::NO_MEMORY:
        break;
    case Type::GENERAL:
        break;
    case Type::CORE:
        break;
    case Type::IO:
        break;
    case Type::GL:
        new (&gl_err_msg) std::string(std::move(err.gl_err_msg));
        break;
    case Type::ERROR_CODE:
        new (&ec) std::error_code(std::move(err.ec));
        break;
    default:
        assert(false);
    }
}

Error& Error::operator=(const Error& err) {
    if (&err == this) return *this;
    this->~Error();
    new (this) Error(err);
    return *this;
}

Error& Error::operator=(Error&& err) noexcept {
    if (&err == this) return *this;
    this->~Error();
    new (this) Error(std::move(err));
    return *this;
}

Error::~Error() {
    switch (ty) {
    case Type::INVALID:
        break;
    case Type::NO_MEMORY:
        break;
    case Type::GENERAL:
        break;
    case Type::CORE:
        break;
    case Type::IO:
        break;
    case Type::GL:
        gl_err_msg.~basic_string();
        break;
    case Type::ERROR_CODE:
        ec.~error_code();
        break;
    default:
        assert(false);
    }
}

void print(rses es) {
    for (const auto& err : es.err_stack) {
        std::cerr << err.str() << '\n';
    }
}

} // namespace err