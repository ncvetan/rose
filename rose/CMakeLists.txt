include_directories("./include")

set(SOURCES 
    "include/rose/app.hpp"
    "include/rose/app_state.hpp"
    "include/rose/camera.hpp"
    "include/rose/entities.hpp"
    "include/rose/gui.hpp"
    "include/rose/lighting.hpp"
    "include/rose/model.hpp"
    "include/rose/texture.hpp"
    "include/rose/core/core.hpp"
    "include/rose/core/err.hpp"
    "include/rose/core/types.hpp"

    "source/rose/app.cpp"
    "source/rose/camera.cpp"
    "source/rose/entities.cpp"
    "source/rose/gui.cpp"
    "source/rose/lighting.cpp"
    "source/rose/model.cpp"
    "source/rose/texture.cpp"
    "source/rose/core/err.cpp"
    "source/rose/core/types.cpp"
)

option(USE_OPENGL "OpenGL Rendering" ON)

if (USE_OPENGL)

    list(APPEND SOURCES 
    "include/rose/gl/platform.hpp" 
    "include/rose/gl/render.hpp"
    "include/rose/gl/shader.hpp"
    "include/rose/gl/structs.hpp"

    "source/rose/gl/platform.cpp"
    "source/rose/gl/render.cpp"
    "source/rose/gl/shader.cpp"
    "source/rose/gl/structs.cpp"
    )
endif()

add_compile_definitions(SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_compile_definitions("$<$<CONFIG:Release>:ROSE_RELEASE_BUILD>")

add_library(rose_lib STATIC ${SOURCES})
add_executable(rose "main.cpp")

if (USE_OPENGL)
    target_compile_definitions(rose PUBLIC USE_OPENGL)
    target_compile_definitions(rose_lib PUBLIC USE_OPENGL)
endif()

target_include_directories(rose_lib PUBLIC ${DEPS_INCLUDE_DIRS})
target_link_libraries(rose_lib PUBLIC ${DEPS_LIBRARIES})
target_link_libraries(rose PUBLIC rose_lib)

