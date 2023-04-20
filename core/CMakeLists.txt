include_directories("include")

find_package(OpenGL REQUIRED)

if (NOT OPENGL_FOUND)         
    message(WARNING "OpenGL Not Found!")
endif()

find_package(glm CONFIG REQUIRED)
message(STATUS "GLM has been included")
find_package(glfw3 CONFIG REQUIRED)
message(STATUS "GLFW has been included")
find_package(glad CONFIG REQUIRED)
message(STATUS "GLAD has been included")
find_package(Stb REQUIRED)
message(STATUS "STB has been included at ${Stb_INCLUDE_DIR}")
find_package(spdlog CONFIG REQUIRED)
message(STATUS "spdlog has been included")

set(EXTERNAL_LIBS opengl32 glm::glm glfw glad::glad spdlog::spdlog)

add_library(rose_lib
    "src/shader.cpp"
    "src/camera.cpp"
    "src/texture.cpp"
    "src/window_manager.cpp"
    "src/logger.cpp"

    "include/shader.hpp"
    "include/camera.hpp"
    "include/texture.hpp"
    "include/window_manager.hpp"
    "include/logger.hpp"
)

set_property(TARGET rose_lib PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET rose_lib PROPERTY CXX_STANDARD 17)
target_link_libraries(rose_lib PUBLIC ${EXTERNAL_LIBS})

add_executable(rose
    "main.cpp"
)

target_include_directories(rose PUBLIC ${Stb_INCLUDE_DIR})
target_link_libraries(rose PUBLIC rose_lib ${EXTERNAL_LIBS})