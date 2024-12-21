include_directories("./include")

add_library(rose_lib STATIC
    "source/rose/application.cpp"
    "source/rose/camera.cpp"
    "source/rose/err.cpp"
    "source/rose/gui.cpp"
    "source/rose/lighting.cpp"
    "source/rose/logger.cpp"
    "source/rose/math.cpp"
    "source/rose/model.cpp"
    "source/rose/object.cpp"
    "source/rose/shader.cpp"
    "source/rose/texture.cpp"
    "source/rose/window.cpp"

    "include/rose/alias.hpp"
    "include/rose/application.hpp"
    "include/rose/camera.hpp"
    "include/rose/err.hpp"
    "include/rose/gui.hpp"
    "include/rose/lighting.hpp"
    "include/rose/logger.hpp"
    "include/rose/math.hpp"
    "include/rose/model.hpp"
    "include/rose/object.hpp"
    "include/rose/shader.hpp"
    "include/rose/texture.hpp"
    "include/rose/window.hpp"
)

target_include_directories(rose_lib PUBLIC ${DEPS_INCLUDE_DIRS})
target_link_libraries(rose_lib PUBLIC ${DEPS_LIBRARIES})

add_executable(rose "main.cpp")
target_link_libraries(rose PUBLIC rose_lib)

target_compile_definitions(rose PUBLIC OPENGL)

add_compile_definitions(SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_compile_definitions("$<$<CONFIG:Release>:ROSE_RELEASE_BUILD>")