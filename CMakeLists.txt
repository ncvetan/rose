cmake_minimum_required (VERSION 3.4)

project ("rose" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

include(InstallRequiredSystemLibraries)

add_subdirectory ("deps")
add_subdirectory ("rose")
