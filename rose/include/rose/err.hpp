#ifndef ROSE_INCLUDE_ERR
#define ROSE_INCLUDE_ERR

#include <format>
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

    rses error_code(const std::error_code& ec);
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
        ERROR_CODE,
    } ty = Type::INVALID;

    std::string msg;

    union {
        int errcode;
        std::string gl_err_msg;
        std::error_code ec;
        std::vector<Error> stack;
    };

    Error() {};
    Error(const Error&);
    Error(Error&&) noexcept;
    Error& operator=(const Error&);
    Error& operator=(Error&&) noexcept;
    ~Error();

    std::string str() const;
};

void print(rses es);

} // namespace err

#endif