#include <rose/core/err.hpp>

#include <cassert>
#include <iostream>
#include <sstream>

namespace err {

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
    err.ty = Error::Type::GL;
    err.msg = std::move(errmsg);
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
        ss << "OpenGL error:\n    " << msg;
        break;
    default:
        assert(false);
        ss << "<<Error enum out of bounds>>";
        break;
    }
    return ss.str();
}

} // namespace err