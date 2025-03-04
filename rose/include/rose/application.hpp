#ifndef ROSE_INCLUDE_APPLICATION
#define ROSE_INCLUDE_APPLICATION

#include <rose/core/err.hpp>
#include <rose/window.hpp>

#include <concepts>
#include <optional>

namespace rose {

template <class T>
concept platform = requires(T t) {
    { t.init() } -> std::same_as<std::optional<rses>>;
    { t.run() } -> std::same_as<void>;
    { t.finish() } -> std::same_as<void>;
};

template <platform T>
struct RoseApp {

    inline std::optional<rses> init() {
        std::optional<rses> err = window.init();
        if (err) {
            return err.value().general("Unable to initialize application");
        }
        return std::nullopt;
    }

    inline void run() { window.run(); }

    inline void finish() { window.finish(); }

    T window;
};
} // namespace rose

#endif