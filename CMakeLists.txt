cmake_minimum_required(VERSION 4.1)

# ===============================================================================
# Extract project version from git tag
# ===============================================================================
find_package(Git REQUIRED)

execute_process(
  COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  OUTPUT_VARIABLE GIT_TAG
  OUTPUT_STRIP_TRAILING_WHITESPACE)

string(REGEX REPLACE "^[vV]" "" TAG_VERSION "${GIT_TAG}")
string(REGEX REPLACE "\-[Rr][Cc][0-9]+" "" TAG_VERSION "${TAG_VERSION}")

project(
  vkBasalt
  VERSION "${TAG_VERSION}"
  HOMEPAGE_URL https://github.com/Gawrjuschin/vkBasalt
  DESCRIPTION
    "vkBasalt is a vulkan layer library injects effects into your games."
  LANGUAGES C CXX)

# ===============================================================================
# C/C++ standarts requirements
# ===============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ===============================================================================
# Dependencies
# ===============================================================================
find_package(SPIRV-Headers REQUIRED)
find_package(
  Vulkan
  COMPONENTS glslc
  REQUIRED)
find_package(X11 REQUIRED)

# ===============================================================================
# Project sources
# ===============================================================================
include(GNUInstallDirs) # includes here to generate config.cpp.in from it's vars

add_subdirectory(src/)

# ===============================================================================
# Install
# ===============================================================================
install(FILES "${CMAKE_CURRENT_LIST_DIR}/config/vkBasalt.conf"
        DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}")

set(vkBasalt_LIBRARY_OUTPUT_NAME
    "${CMAKE_SHARED_LIBRARY_PREFIX}vkBasalt${CMAKE_SHARED_LIBRARY_SUFFIX}")

configure_file("${CMAKE_CURRENT_LIST_DIR}/config/VkLayer_vkBasalt.json.in"
               "${CMAKE_CURRENT_BINARY_DIR}/VkLayer_vkBasalt.json" @ONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/VkLayer_vkBasalt.json"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/vulkan/implicit_layer.d")

install(
  TARGETS vkBasalt_vkBasalt
  EXPORT ${PROJECT_NAME}-targets
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/vkBasalt")

# ===============================================================================
# Packaging
# ===============================================================================
include(packaging/CPackConfig.cmake)
