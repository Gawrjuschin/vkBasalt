cmake_minimum_required(VERSION 3.31)

project(vkbasalt LANGUAGES C CXX)

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

include(GNUInstallDirs)

# TODO: only if with_so
add_subdirectory(src/)

# ##############################################################################
# Install
# ##############################################################################
set(VKBASALT_LIBRARY_OUTPUT_NAME
    "${CMAKE_SHARED_LIBRARY_PREFIX}vkbasalt${CMAKE_SHARED_LIBRARY_SUFFIX}")

# TODO: only if with_so
configure_file(${CMAKE_CURRENT_LIST_DIR}/config/VkLayer_vkBasalt.json.in
               ${CMAKE_CURRENT_BINARY_DIR}/VkLayer_vkBasalt.json @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/VkLayer_vkBasalt.json
        DESTINATION "${CMAKE_INSTALL_DATADIR}/vulkan/implicit_layer.d")

install(FILES ${CMAKE_CURRENT_LIST_DIR}/config/vkBasalt.conf
        DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_SYSCONFDIR}")

install(
  TARGETS vkbasalt_vkbasalt
  EXPORT ${PROJECT_NAME}-targets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}) # TODO: append_libdir support

install(
  EXPORT ${PROJECT_NAME}-targets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  NAMESPACE ${PROJECT_NAME}::
  FILE ${PROJECT_NAME}-config.cmake)
