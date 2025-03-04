include_directories("./include")

add_library(rose_lib STATIC
    "include/rose/application.hpp"
    "include/rose/camera.hpp"
    "include/rose/entities.hpp"
    "include/rose/gui.hpp"
    "include/rose/lighting.hpp"
    "include/rose/model.hpp"
    "include/rose/texture.hpp"
    "include/rose/window.hpp"
    "include/rose/core/alias.hpp"
    "include/rose/core/err.hpp"
    "include/rose/core/math.hpp"
    "include/rose/gl/gl.hpp"
    "include/rose/gl/shader.hpp"
    "include/rose/gl/structs.hpp"

    "source/rose/application.cpp"
    "source/rose/camera.cpp"
    "source/rose/entities.cpp"
    "source/rose/gui.cpp"
    "source/rose/lighting.cpp"
    "source/rose/model.cpp"
    "source/rose/texture.cpp"
    "source/rose/window.cpp"
    "source/rose/core/err.cpp"
    "source/rose/core/math.cpp"
    "source/rose/gl/gl.cpp"
    "source/rose/gl/shader.cpp"
    "source/rose/gl/structs.cpp"
)

target_include_directories(rose_lib PUBLIC ${DEPS_INCLUDE_DIRS})
target_link_libraries(rose_lib PUBLIC ${DEPS_LIBRARIES})

add_executable(rose "main.cpp" "source/rose/gl/structs.cpp")
target_link_libraries(rose PUBLIC rose_lib)

target_compile_definitions(rose PUBLIC OPENGL)

add_compile_definitions(SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_compile_definitions("$<$<CONFIG:Release>:ROSE_RELEASE_BUILD>")