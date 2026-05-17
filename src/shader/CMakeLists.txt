cmake_minimum_required(VERSION 3.31)

# ===============================================================================
# Fetch SMAA
# ===============================================================================
include(FetchContent)

FetchContent_Declare(
  smaa
  GIT_REPOSITORY https://github.com/iryoku/smaa.git
  GIT_TAG master)

FetchContent_MakeAvailable(SMAA)

# ===============================================================================
# Shaders generation macro
# ===============================================================================
macro(target_generate_shaders target)
  set(FILE_NAMES "${ARGN}")
  set(headers_list "")

  foreach(FILE IN LISTS FILE_NAMES)
    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FILE}.h"
      COMMAND
        Vulkan::glslc -I "${smaa_SOURCE_DIR}" -mfmt=num
        "${CMAKE_CURRENT_LIST_DIR}/${FILE}" -o
        "${CMAKE_CURRENT_BINARY_DIR}/${FILE}.h"
      VERBATIM)
    list(APPEND headers_list "${CMAKE_CURRENT_BINARY_DIR}/${FILE}.h")
  endforeach()

  target_sources(
    ${target}
    PUBLIC FILE_SET
           smaa_files
           TYPE
           HEADERS
           BASE_DIRS
           ${smaa_SOURCE_DIR}
           FILES
           ${smaa_SOURCE_DIR}/SMAA.hlsl
           ${smaa_SOURCE_DIR}/Textures/AreaTex.h
           ${smaa_SOURCE_DIR}/Textures/SearchTex.h
    PUBLIC FILE_SET
           generated_shaders
           TYPE
           HEADERS
           BASE_DIRS
           ${CMAKE_CURRENT_BINARY_DIR}
           FILES
           "${headers_list}")
endmacro()

# ===============================================================================
# Generated shaders library
# ===============================================================================
add_library(vkBasalt_shader INTERFACE)

target_sources(
  vkBasalt_shader
  PUBLIC FILE_SET
         headers
         TYPE
         HEADERS
         BASE_DIRS
         ${CMAKE_CURRENT_LIST_DIR}
         FILES
         ${CMAKE_CURRENT_LIST_DIR}/fxaa3_11.h
         ${CMAKE_CURRENT_LIST_DIR}/smaa_settings.h)

target_generate_shaders(
  vkBasalt_shader
  cas.frag
  deband.frag
  dls.frag
  full_screen_triangle.vert
  fxaa.frag
  lut.frag
  smaa_blend.frag
  smaa_blend.vert
  smaa_edge.vert
  smaa_edge.vert
  smaa_edge_color.frag
  smaa_edge_luma.frag
  smaa_neighbor.frag
  smaa_neighbor.vert)
