cmake_minimum_required(VERSION 3.31)

# ===============================================================================
# Reshade Came with Stb dependency
# ===============================================================================
# TODO: better solution
cmake_policy(SET CMP0169 OLD)

include(FetchContent)

FetchContent_Declare(
  reshade
  GIT_REPOSITORY "https://github.com/crosire/reshade.git"
  GIT_TAG "v4.9.0") # max supported by vkBasalt version now

FetchContent_Populate(reshade)

add_library(vkBasalt_reshade OBJECT)

target_sources(
  vkBasalt_reshade
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/stb_impl.c
  PUBLIC FILE_SET
         reshade_headers
         TYPE
         HEADERS
         BASE_DIRS
         ${reshade_SOURCE_DIR}/source
         FILES
         ${reshade_SOURCE_DIR}/source/effect_expression.hpp
         ${reshade_SOURCE_DIR}/source/effect_module.hpp
  PUBLIC FILE_SET
         stb_image_dds_headers
         TYPE
         HEADERS
         BASE_DIRS
         ${reshade_SOURCE_DIR}/deps/stb_image_dds
         FILES
         ${reshade_SOURCE_DIR}/deps/stb_image_dds/stb_image_dds.h
  PUBLIC FILE_SET
         stb_headers
         TYPE
         HEADERS
         BASE_DIRS
         ${reshade_SOURCE_DIR}/deps/stb
         FILES
         ${reshade_SOURCE_DIR}/deps/stb/stb_image.h
         ${reshade_SOURCE_DIR}/deps/stb/stb_image_resize.h)

set_target_properties(
  vkBasalt_reshade
  PROPERTIES POSITION_INDEPENDENT_CODE TRUE # Enables PIC
             C_VISIBILITY_PRESET hidden # Hides symbols
             CXX_VISIBILITY_PRESET hidden # Hides symbols
             VISIBILITY_INLINES_HIDDEN ON # Hides inlined functions
)
