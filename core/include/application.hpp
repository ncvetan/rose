#pragma once

#include <memory>
#include <window.hpp>

namespace rose
{
    class RoseApp
    {
      public:
        enum class Platform
        {
            None,
            OpenGL
        };

        RoseApp(Platform platform);

        bool init();
        void run();
        void shutdown();

        Platform platform = Platform::None;
        std::unique_ptr<Window> window = nullptr;
        bool is_running = false;
    };
} // namespace rose
