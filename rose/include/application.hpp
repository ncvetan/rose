#ifndef ROSE_INCLUDE_APPLICATION
#define ROSE_INCLUDE_APPLICATION

#include <window.hpp>
#include <concepts>

namespace rose {

template <class T>
concept platform = requires(T t) {
    { t.init() } -> std::same_as<bool>;
    { t.update() } -> std::same_as<void>;
    { t.destroy() } -> std::same_as<void>;
};

template <platform T>
class RoseApp {
  public:
    RoseApp() = default;

    bool init() { return window.init(); }

    void run() {
        is_running = true;
        while (is_running) {
            window.update();
        }
    }

    void shutdown() { window.destroy(); }

    T window;
    bool is_running = false;
};
} // namespace rose

#endif