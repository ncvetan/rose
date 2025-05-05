#include <rose/app.hpp>
#include <rose/core/err.hpp>

#ifdef USE_OPENGL
#include <rose/gl/platform.hpp>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int main() {

    /*
    A graphics API can be selected at compile time within Rose/CMakeLists.txt which resolves which 'platform'
    is used in the program. A platform simply defines an API specific initialization function, a function called 
    within the main loop, and a deinitialization function.

    Ideally these API abstractions can be made more granular in the future to reduce redundancy
    between platforms, however this will likely be done once development starts to support 
    another API
    
    */
#ifdef USE_OPENGL
    gl::Platform platform;
#else
    return -1;
#endif

    RoseApp app;
    rses err = app.init(platform);

    if (err) { 
        err::print(err);    
        return -1;
    }

    app.run(platform);
    app.finish(platform);

    return 0;
}
