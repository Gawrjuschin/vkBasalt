cmake_minimum_required(VERSION 3.31)

# ===============================================================================
# Fetch SMAA
# ===============================================================================
include(FetchContent)

FetchContent_Declare(
  smaa
  GIT_REPOSITORY https://github.com/iryoku/smaa.git
  GIT_TAG master)

# TODO: change to repo mentioned in this gist or another repo with fxaa file
FetchContent_Declare(
  fxaa
  URL "https://gist.github.com/kosua20/0c506b81b3812ac900048059d2383126/raw/9c1f4b97ea5a2ac4bf632fff509f64d7dff0e96a/gistfile1.txt"
  DOWNLOAD_NO_EXTRACT TRUE
  PATCH_COMMAND ${CMAKE_COMMAND} -E rename <SOURCE_DIR>/gistfile1.txt
                <SOURCE_DIR>/fxaa3_11.h)

FetchContent_MakeAvailable(smaa fxaa)

# ===============================================================================
# Shaders generation macro
# ===============================================================================
function(target_generate_shaders target)
  set(FILE_NAMES "${ARGN}")
  set(headers_list "")

  foreach(FILE IN LISTS FILE_NAMES)
    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/${FILE}.spv"
      COMMAND
        Vulkan::glslc -I "${smaa_SOURCE_DIR}" -I "${fxaa_SOURCE_DIR}" -mfmt=num
        "${CMAKE_CURRENT_LIST_DIR}/${FILE}" -o
        "${CMAKE_CURRENT_BINARY_DIR}/generated/${FILE}.spv"
      VERBATIM CODEGEN)
    list(APPEND headers_list
         "${CMAKE_CURRENT_BINARY_DIR}/generated/${FILE}.spv")
  endforeach()

  target_sources(
    ${target}
    PUBLIC FILE_SET
           generated_shaders
           TYPE
           HEADERS
           BASE_DIRS
           ${CMAKE_CURRENT_BINARY_DIR}/generated
           FILES
           "${headers_list}")
endfunction()

# ===============================================================================
# Generated shaders library
# ===============================================================================
add_library(vkBasalt_shader INTERFACE)

target_sources(
  vkBasalt_shader
  PUBLIC FILE_SET
         smaa_textures
         TYPE
         HEADERS
         BASE_DIRS
         ${smaa_SOURCE_DIR}
         FILES
         ${smaa_SOURCE_DIR}/Textures/AreaTex.h
         ${smaa_SOURCE_DIR}/Textures/SearchTex.h
  PRIVATE FILE_SET
          shaders_files
          TYPE
          HEADERS
          BASE_DIRS
          ${CMAKE_CURRENT_LIST_DIR}
          FILES
          ${CMAKE_CURRENT_LIST_DIR}/cas.frag
          ${CMAKE_CURRENT_LIST_DIR}/deband.frag
          ${CMAKE_CURRENT_LIST_DIR}/dls.frag
          ${CMAKE_CURRENT_LIST_DIR}/full_screen_triangle.vert
          ${CMAKE_CURRENT_LIST_DIR}/fxaa.frag
          ${CMAKE_CURRENT_LIST_DIR}/lut.frag
          ${CMAKE_CURRENT_LIST_DIR}/smaa_blend.frag
          ${CMAKE_CURRENT_LIST_DIR}/smaa_blend.vert
          ${CMAKE_CURRENT_LIST_DIR}/smaa_edge.vert
          ${CMAKE_CURRENT_LIST_DIR}/smaa_edge_color.frag
          ${CMAKE_CURRENT_LIST_DIR}/smaa_edge_luma.frag
          ${CMAKE_CURRENT_LIST_DIR}/smaa_neighbor.frag
          ${CMAKE_CURRENT_LIST_DIR}/smaa_neighbor.vert
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

target_include_directories(
  vkBasalt_shader
  INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>")
