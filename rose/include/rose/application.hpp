#ifndef ROSE_INCLUDE_APPLICATION
#define ROSE_INCLUDE_APPLICATION

#include <rose/err.hpp>
#include <rose/window.hpp>

#include <concepts>
#include <optional>

namespace rose {

template <class T>
concept platform = requires(T t) {
    { t.init() } -> std::same_as<std::optional<rses>>;
    { t.update() } -> std::same_as<void>;
    { t.destroy() } -> std::same_as<void>;
};

template <platform T>
class RoseApp {
  public:
    RoseApp() = default;

    std::optional<rses> init() {
        std::optional<rses> err = window.init();
        if (err) {
            return err.value().general("Unable to initialize application");
        }
        return std::nullopt;
    }

    void run() { window.update(); }

    void shutdown() { window.destroy(); }

  private:
    T window;
};
} // namespace rose

#endif