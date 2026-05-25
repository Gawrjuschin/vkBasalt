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
