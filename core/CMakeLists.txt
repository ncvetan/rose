include_directories("include")

find_package(OpenGL REQUIRED)

if (NOT OPENGL_FOUND)         
    message(WARNING "OpenGL Not Found!")
endif()

find_package(glm CONFIG REQUIRED)
message(STATUS "GLM included at ${GLM_INCLUDE_DIR}")
find_package(glfw3 CONFIG REQUIRED)
message(STATUS "GLFW included at ${GLFW_INCLUDE_DIR}")
find_package(glad CONFIG REQUIRED)
message(STATUS "GLAD included at ${GLAD_INCLUDE_DIR}")
find_package(Stb REQUIRED)
message(STATUS "STB included at ${Stb_INCLUDE_DIR}")

set(EXTERNAL_LIBS opengl32 glm::glm glfw glad stb)

add_library(rose_lib
    "src/shader.cpp"
    "src/camera.cpp"
    "src/texture.cpp"

    "include/shader.hpp"
    "include/camera.hpp"	
    "include/texture.hpp"
)

set_property(TARGET rose_lib PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET rose_lib PROPERTY CXX_STANDARD 17)


add_executable(rose
    "main.cpp"
)

target_include_directories(rose PUBLIC ${Stb_INCLUDE_DIR})
target_link_libraries(rose PUBLIC rose_lib ${LIBS})