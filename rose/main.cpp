#include <rose/app.hpp>
#include <rose/core/err.hpp>

#ifdef OPENGL
#include <rose/gl/platform.hpp>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <print>

int main() {

#ifdef OPENGL
    gl::Platform platform;
#else
    return -1;
#endif

    RoseApp application;
    std::optional<rses> err = application.init(platform);

    if (err) { 
        err::print(err.value());    
        return -1;
    }

    application.run(platform);
    application.finish(platform);
    return 0;
}
