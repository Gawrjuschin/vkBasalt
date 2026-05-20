cmake_minimum_required(VERSION 3.31)

# ===============================================================================
# X11 support library hides includes
# ===============================================================================
add_library(vkBasalt_keyboard_input OBJECT)

target_sources(
  vkBasalt_keyboard_input
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/keyboard_input_x11.hpp
          ${CMAKE_CURRENT_LIST_DIR}/src/keyboard_input_x11.cpp
          ${CMAKE_CURRENT_LIST_DIR}/src/keyboard_input.cpp
  PUBLIC FILE_SET
         headers
         TYPE
         HEADERS
         BASE_DIRS
         ${CMAKE_CURRENT_LIST_DIR}/include
         FILES
         ${CMAKE_CURRENT_LIST_DIR}/include/keyboard_input.hpp)

# TODO: optional X11 linkage
target_link_libraries(vkBasalt_keyboard_input PUBLIC X11::X11 vkBasalt_logger)

set_target_properties(
  vkBasalt_keyboard_input
  PROPERTIES POSITION_INDEPENDENT_CODE TRUE # Enables PIC
             C_VISIBILITY_PRESET hidden # Hides symbols
             CXX_VISIBILITY_PRESET hidden # Hides symbols
             VISIBILITY_INLINES_HIDDEN ON # Hides inlined functions
)
