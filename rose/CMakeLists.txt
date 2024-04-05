include_directories("./include")

add_library(rose_lib STATIC
    "source/application.cpp"
    "source/camera.cpp"
    "source/err.cpp"
    "source/event.cpp"
    "source/logger.cpp"
    "source/mesh.cpp"
    "source/shader.cpp"
    "source/texture.cpp"
    "source/window.cpp"

    "include/application.hpp"
    "include/camera.hpp"
    "include/err.hpp"
    "include/event.hpp"
    "include/logger.hpp"
    "include/mesh.hpp"
    "include/shader.hpp"
    "include/texture.hpp"
    "include/window.hpp"
)

target_include_directories(rose_lib PUBLIC ${DEPS_INCLUDE_DIRS})
target_link_libraries(rose_lib PUBLIC ${DEPS_LIBRARIES})

add_executable(rose "main.cpp")
target_link_libraries(rose PUBLIC rose_lib)

target_compile_definitions(rose PUBLIC OPENGL)

add_compile_definitions(SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_compile_definitions("$<$<CONFIG:Release>:ROSE_RELEASE_BUILD>")