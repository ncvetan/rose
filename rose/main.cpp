#include <rose/app.hpp>
#include <rose/core/err.hpp>

#ifdef USE_OPENGL
#include <rose/backends/gl/backend.hpp>
#else
static_assert("no backend selected");
#endif 

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int main() {

    /*
    A graphics API can be selected at compile time within Rose/CMakeLists.txt which resolves which 'backend'
    is used in the program. A backend simply defines an API specific initialization function, a function called 
    within the main loop, and a deinitialization function.

    Ideally these API abstractions can be made more granular in the future to reduce redundancy
    between backends, however this will likely be done once development starts to support 
    another API
    
    */
#ifdef USE_OPENGL
    gl::Backend backend;
#else
    static_assert("no backend selected");
    return -1;
#endif

    RoseApp app;
    rses err = app.init(backend);

    if (err) { 
        err::print(err);    
        return -1;
    }

    app.run(backend);
    app.finish(backend);

    return 0;
}
