// =============================================================================
//   structures for Rose's error handling system
// =============================================================================

#ifndef ROSE_INCLUDE_CORE_ERR
#define ROSE_INCLUDE_CORE_ERR

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
    rses general(std::string&& fmt, Args&&... args) {
        return general(std::vformat(std::move(fmt), std::make_format_args(args...)));
    }

    rses core(std::string&& errmsg);
    template <typename... Args>
    rses core(std::string&& fmt, Args&&... args) {
        return core(std::vformat(std::move(fmt), std::make_format_args(args...)));
    }

    rses io(std::string&& errmsg);
    template <typename... Args>
    rses io(std::string&& fmt, Args&&... args) {
        return io(std::vformat(std::move(fmt), std::make_format_args(args...)));
    }

    rses gl(std::string&& errmsg);
    template <typename... Args>
    rses gl(std::string&& fmt, Args&&... args) {
        return gl(std::vformat(std::move(fmt), std::make_format_args(args...)));
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
#ifndef ROSE_RELEASE_BUILD
    for (const auto& err : es.err_stack) {
        std::println("{}", err.str());
    }
#endif
}

} // namespace err

#endif