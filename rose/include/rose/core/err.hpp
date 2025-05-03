// =============================================================================
//   structures for Rose's error handling system
// =============================================================================

#ifndef ROSE_INCLUDE_CORE_ERR
#define ROSE_INCLUDE_CORE_ERR

#include <rose/core/core.hpp>

#include <format>
#include <print>
#include <string>
#include <vector>

namespace err {
struct Handle;
}

using rses = err::Handle; // rose error stack

namespace err {

struct Error;

struct Handle {

    rses no_memory();

    rses general(std::string&& errmsg);
    template <typename... Args>
    rses general(std::string_view fmt, Args&&... args) {
        return general(std::vformat(fmt, std::make_format_args(args...)));
    }

    rses core(std::string&& errmsg);
    template <typename... Args>
    rses core(std::string_view fmt, Args&&... args) {
        return core(std::vformat(fmt, std::make_format_args(args...)));
    }

    rses io(std::string&& errmsg);
    template <typename... Args>
    rses io(std::string_view fmt, Args&&... args) {
        return io(std::vformat(fmt, std::make_format_args(args...)));
    }

    rses gl(std::string&& errmsg);
    template <typename... Args>
    rses gl(std::string_view fmt, Args&&... args) {
        return gl(std::vformat(fmt, std::make_format_args(args...)));
    }

    explicit operator bool() const {
        return !err_stack.empty();
    }

    rses add_error(Error&& err);

    std::vector<Error> err_stack;
};

struct Error {

    enum class Type {
        INVALID,
        NO_MEMORY,
        GENERAL,
        CORE,
        IO,
        GL,
        N_TYPES
    };
    
    Type ty = Type::INVALID;
    std::string msg;

    std::string str() const;
};

inline void print(rses es) {
    for (i64 idx = es.err_stack.size() - 1; idx >= 0; --idx) {
        std::println("{}", es.err_stack[idx].str());
    }
}

} // namespace err

#endif