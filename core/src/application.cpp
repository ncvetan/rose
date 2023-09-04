#include <application.hpp>
#include <logger.hpp>
#include <window.hpp>

#include <GL/glew.h>

namespace rose
{
    RoseApp::RoseApp(Platform platform) : window(nullptr), platform(platform), is_running(false){};

    bool RoseApp::init()
    {
        switch (platform)
        {
        case RoseApp::Platform::None:
            LOG_ERROR("No platform has been specified");
            return false;
            break;
        case RoseApp::Platform::OpenGL:
        {
            window = std::make_unique<rose::WindowGLFW>();
            if (!window->init())
            {
                LOG_ERROR("Error initializing window");
                return false;
            }
            LOG_INFO("Successfully created OpenGL application");
            break;
        }
        default:
            break;
        }
        return true;
    }

    void RoseApp::run()
    {
        is_running = true;
        while (is_running)
        {
            window->update();
        }
    }

    void RoseApp::shutdown() { return; }
} // namespace rose
