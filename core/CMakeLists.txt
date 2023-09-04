include_directories("./include")

add_library(rose_lib STATIC
    "src/application.cpp"
    "src/camera.cpp"
    "src/event.cpp"
    "src/logger.cpp"
    "src/shader.cpp"
    "src/texture.cpp"
    "src/window.cpp"

    "include/application.hpp"
    "include/camera.hpp"
    "include/event.hpp"
    "include/logger.hpp"
    "include/shader.hpp"
    "include/texture.hpp"
    "include/window.hpp"
)

target_include_directories(rose_lib PUBLIC ${DEPS_INCLUDE_DIRS})
target_link_libraries(rose_lib PUBLIC ${DEPS_LIBRARIES})

add_executable(rose "main.cpp")
target_link_libraries(rose PUBLIC rose_lib)
add_compile_definitions("$<$<CONFIG:Release>:ROSE_RELEASE_BUILD>")