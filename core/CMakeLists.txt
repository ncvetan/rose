include_directories("include")
include_directories("${PROJECT_SOURCE_DIR}/deps/stb")

set(DEPENDENCIES glm::glm_static spdlog::spdlog libglew_static glfw)
set(core_lib "${PROJECT_NAME}_lib")

add_library(core_lib STATIC
    "src/shader.cpp"
    "src/camera.cpp"
    "src/texture.cpp"
    "src/window_manager.cpp"
    "src/logger.cpp"
    "src/main_app.cpp"

    "include/shader.hpp"
    "include/camera.hpp"
    "include/texture.hpp"
    "include/window_manager.hpp"
    "include/logger.hpp"
    "include/main_app.hpp"
)

set_property(TARGET core_lib PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET core_lib PROPERTY CXX_STANDARD 17)
target_link_libraries(core_lib PUBLIC DEPENDENCIES)

add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries(${PROJECT_NAME} PUBLIC core_lib DEPENDENCIES)
add_compile_definitions("$<$<CONFIG:Release>:ATLAS_RELEASE_BUILD>")