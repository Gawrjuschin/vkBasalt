cmake_minimum_required(VERSION 3.31)

# TODO: use list argument
macro(generate_shader headers_list name)
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${name}.h"
    COMMAND glslangValidator -V -x "${CMAKE_CURRENT_LIST_DIR}/${name}.glsl" -o
            "${CMAKE_CURRENT_BINARY_DIR}/${name}.h"
    VERBATIM)
  list(APPEND headers_list "${CMAKE_CURRENT_BINARY_DIR}/${name}.h")
endmacro()

set(headers_list "")
generate_shader(headers_list cas.frag)
generate_shader(headers_list deband.frag)
generate_shader(headers_list dls.frag)
generate_shader(headers_list full_screen_triangle.vert)
generate_shader(headers_list fxaa.frag)
generate_shader(headers_list lut.frag)
generate_shader(headers_list smaa_blend.frag)
generate_shader(headers_list smaa_blend.vert)
generate_shader(headers_list smaa_edge.vert)
generate_shader(headers_list smaa_edge.vert)
generate_shader(headers_list smaa_edge_color.frag)
generate_shader(headers_list smaa_edge_luma.frag)
generate_shader(headers_list smaa_neighbor.frag)
generate_shader(headers_list smaa_neighbor.vert)

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
         ${CMAKE_CURRENT_LIST_DIR}/smaa.h
         ${CMAKE_CURRENT_LIST_DIR}/smaa_settings.h
         FILE_SET
         generated
         TYPE
         HEADERS
         BASE_DIRS
         ${CMAKE_CURRENT_BINARY_DIR}
         FILES
         "${headers_list}")

get_target_property(MY_PROJECT_SOURCES vkBasalt_shader HEADER_SET_generated)
message(FATAL_ERROR "${MY_PROJECT_SOURCES}")
