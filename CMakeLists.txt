cmake_minimum_required(VERSION 3.31)

project(
  vkBasalt
  VERSION 0.1.1
  HOMEPAGE_URL https://github.com/Gawrjuschin/vkBasalt
  DESCRIPTION
    "vkBasalt is a vulkan layer library injects effects into your games."
  LANGUAGES C CXX)

# ===============================================================================
# C/C++ standarts requirements
# ===============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ===============================================================================
# Dependencies
# ===============================================================================
find_package(SPIRV-Headers REQUIRED)
find_package(
  Vulkan
  COMPONENTS glslangValidator
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

install(FILES ${CMAKE_CURRENT_LIST_DIR}/config/vkBasalt.conf
        DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_SYSCONFDIR}")

if(VKBASALT_WITH_JSON)
  set(VKBASALT_LIBRARY_OUTPUT_NAME
      "${CMAKE_SHARED_LIBRARY_PREFIX}vkbasalt${CMAKE_SHARED_LIBRARY_SUFFIX}")

  configure_file(${CMAKE_CURRENT_LIST_DIR}/config/VkLayer_vkBasalt.json.in
                 ${CMAKE_CURRENT_BINARY_DIR}/VkLayer_vkBasalt.json @ONLY)

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/VkLayer_vkBasalt.json
          DESTINATION "${CMAKE_INSTALL_DATADIR}/vulkan/implicit_layer.d")
endif()

if(VKBASALT_WITH_SO)
  install(
    TARGETS vkbasalt_vkbasalt
    EXPORT ${PROJECT_NAME}-targets
    DESTINATION ${CMAKE_INSTALL_LIBDIR})

  install(
    EXPORT ${PROJECT_NAME}-targets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    NAMESPACE ${PROJECT_NAME}::
    FILE ${PROJECT_NAME}-config.cmake)
endif()

# ===============================================================================
# Packaging
# ===============================================================================
add_subdirectory(packaging/)
