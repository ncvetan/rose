#pragma once

#include <window.hpp>

namespace rose {
template <typename T>
class RoseApp {
  public:
    RoseApp() = default;

    bool init() { return static_cast<Window<T>*>(&window)->init(); }

    void run() {
        is_running = true;
        while (is_running) {
            static_cast<Window<T>*>(&window)->update();
        }
    }

    void shutdown() { static_cast<Window<T>*>(&window)->destroy(); }

    T window;
    bool is_running = false;
};
} // namespace rose
