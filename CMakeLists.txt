cmake_minimum_required(VERSION 3.31)

project(vkBasalt LANGUAGES C CXX)

# default_options from previous meson: ['c_std=c11', 'cpp_std=c++2a'])
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# TODO: check if required
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(VKBASALT_APPEND_LIBDIR ON)
option(VKBASALT_WITH_SO ON)
option(VKBASALT_WITH_JSON ON)

find_package(SPIRV-Headers CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(X11 REQUIRED)
find_program(GLSLANG names glslangValidator)

add_subdirectory(src/)

# TODO: install scripts and don't forget about default config requires cmake
# generation from a template
