cmake_minimum_required(VERSION 3.31)

# TODO: escaping in cmd
macro(target_generate_shaders target)
  set(FILES_STEMS "${ARGN}")
  set(headers_list "")

  foreach(STEM IN LISTS FILES_STEMS)
    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${STEM}.h"
      COMMAND glslangValidator -V -x "${CMAKE_CURRENT_LIST_DIR}/${STEM}.glsl" -o
              "${CMAKE_CURRENT_BINARY_DIR}/${STEM}.h"
      VERBATIM)
    list(APPEND headers_list "${CMAKE_CURRENT_BINARY_DIR}/${STEM}.h")
  endforeach()

  target_sources(
    ${target}
    PUBLIC FILE_SET
           generated
           TYPE
           HEADERS
           BASE_DIRS
           ${CMAKE_CURRENT_BINARY_DIR}
           FILES
           "${headers_list}")
endmacro()

add_library(vkbasalt_shader INTERFACE)

target_sources(
  vkbasalt_shader
  PUBLIC FILE_SET
         headers
         TYPE
         HEADERS
         BASE_DIRS
         ${CMAKE_CURRENT_LIST_DIR}
         FILES
         ${CMAKE_CURRENT_LIST_DIR}/fxaa3_11.h
         ${CMAKE_CURRENT_LIST_DIR}/smaa.h
         ${CMAKE_CURRENT_LIST_DIR}/smaa_settings.h)

target_generate_shaders(
  vkbasalt_shader
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

set_target_properties(
  vkbasalt_shader
  PROPERTIES POSITION_INDEPENDENT_CODE TRUE # Enables PIC
             C_VISIBILITY_PRESET hidden # Hides symbols
             CXX_VISIBILITY_PRESET hidden # Hides symbols
             VISIBILITY_INLINES_HIDDEN ON # Hides inlined functions
)
