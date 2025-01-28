#include <rose/application.hpp>
#include <rose/err.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <print>

int main() {

#ifdef OPENGL
    rose::RoseApp application = rose::RoseApp<rose::WindowGLFW>();
#else
    std::println("No platform has been defined");
    return -1;
#endif

    std::optional<rses> err = application.init();

    if (err) { 
        err::print(err.value());    
        return -1;
    }

    application.run();
    application.shutdown();
    return 0;
}
