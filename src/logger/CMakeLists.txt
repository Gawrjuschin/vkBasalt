cmake_minimum_required(VERSION 3.31)

add_library(vkBasalt_logger OBJECT)

target_sources(
  vkBasalt_logger
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/logger.cpp
  PUBLIC FILE_SET
         headers
         TYPE
         HEADERS
         BASE_DIRS
         ${CMAKE_CURRENT_LIST_DIR}/include/
         FILES
         ${CMAKE_CURRENT_LIST_DIR}/include/logger.hpp)

set_target_properties(
  vkBasalt_logger
  PROPERTIES POSITION_INDEPENDENT_CODE TRUE # Enables PIC
             C_VISIBILITY_PRESET hidden # Hides symbols
             CXX_VISIBILITY_PRESET hidden # Hides symbols
             VISIBILITY_INLINES_HIDDEN ON # Hides inlined functions
)
