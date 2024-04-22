set(BUILD_STATIC_LIBS true)

add_subdirectory("glew-cmake")
set(GLEW_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/glew-cmake/include")
set(GLEW_LIBRARIES "libglew_static")

add_subdirectory("glfw")
set(GLFW_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/glfw/include")
set(GLFW_LIBRARIES "glfw")

add_subdirectory("glm")
set(GLM_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/glm/glm")
set(GLM_LIBRARIES "glm_static")

add_subdirectory("spdlog")
set(SPDLOG_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include")
set(SPDLOG_LIBRARIES "spdlog")

set(STB_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/stb")

add_subdirectory("assimp")
set(BUILD_SHARED_LIBS false)
set(ASSIMP_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/assimp/include")
set(ASSIMP_LIBRARIES "assimp")

add_library(ImGui STATIC
  "imgui/imgui.h"
  "imgui/imgui_internal.h"
  "imgui/imstb_rectpack.h"
  "imgui/imstb_textedit.h"
  "imgui/imstb_truetype.h"
  "imgui/imconfig.h"
  "imgui/backends/imgui_impl_glfw.h"
  "imgui/backends/imgui_impl_opengl3.h"

  "imgui/imgui.cpp"
  "imgui/imgui_demo.cpp"
  "imgui/imgui_draw.cpp"
  "imgui/imgui_tables.cpp"
  "imgui/imgui_widgets.cpp"
  "imgui/backends/imgui_impl_glfw.cpp"
  "imgui/backends/imgui_impl_opengl3.cpp"
)

target_include_directories(ImGui PUBLIC "imgui" ${GLFW_INCLUDE_DIRS})
target_link_libraries(ImGui PUBLIC ${GLFW_LIBRARIES})

set(IMGUI_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
set(IMGUI_LIBRARIES "ImGui")

set(DEPS_INCLUDE_DIRS ${GLEW_INCLUDE_DIRS} ${GLFW_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS} ${SPDLOG_INCLUDE_DIRS} ${STB_INCLUDE_DIRS} ${ASSIMP_INCLUDE_DIRS} ${IMGUI_INCLUDE_DIRS} CACHE STRING "" FORCE)
set(DEPS_LIBRARIES ${GLEW_LIBRARIES} ${GLFW_LIBRARIES} ${GLM_LIBRARIES} ${SPDLOG_LIBRARIES} ${ASSIMP_LIBRARIES} ${IMGUI_LIBRARIES} CACHE STRING "" FORCE)