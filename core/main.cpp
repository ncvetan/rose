#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <application.hpp>
#include <logger.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int main()
{
    rose::Logger::initialize();
    rose::RoseApp application = rose::RoseApp(rose::RoseApp::Platform::OpenGL);
    if (!application.init())
    {
        LOG_ERROR("Rose failed to initialize!");
        return -1;
    }
    application.run();
    application.shutdown();
    return 0;
}
