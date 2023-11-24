#include <application.hpp>
#include <logger.hpp>
#include <window.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int main() {
    rose::Logger::init();

#ifdef OPENGL
    rose::RoseApp application = rose::RoseApp<rose::WindowGLFW>();
#else
    LOG_ERROR("No platform has been defined");
    return -1;
#endif

    if (!application.init()) {
        LOG_ERROR("Rose failed to initialize!");
        return -1;
    }
    application.run();
    application.shutdown();
    return 0;
}
