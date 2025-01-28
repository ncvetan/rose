cmake_minimum_required (VERSION 3.4)

project ("rose" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

include(InstallRequiredSystemLibraries)

add_subdirectory ("deps")

if(MSVC)
    add_compile_options(/EHsc /W4)
endif()

add_subdirectory ("rose")
