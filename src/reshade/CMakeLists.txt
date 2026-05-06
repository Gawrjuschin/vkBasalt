cmake_minimum_required(VERSION 3.31)

add_library(vkBasalt_reshade OBJECT)

target_sources(
  vkBasalt_reshade
  PUBLIC FILE_SET
         HEADERS
         FILES
         ${CMAKE_CURRENT_LIST_DIR}/GLSL.std.450.h
         ${CMAKE_CURRENT_LIST_DIR}/spirv.hpp
         ${CMAKE_CURRENT_LIST_DIR}/effect_codegen.hpp
         ${CMAKE_CURRENT_LIST_DIR}/effect_expression.hpp
         ${CMAKE_CURRENT_LIST_DIR}/effect_lexer.hpp
         ${CMAKE_CURRENT_LIST_DIR}/effect_module.hpp
         ${CMAKE_CURRENT_LIST_DIR}/effect_parser.hpp
         ${CMAKE_CURRENT_LIST_DIR}/effect_preprocessor.hpp
         ${CMAKE_CURRENT_LIST_DIR}/effect_symbol_table.hpp
         ${CMAKE_CURRENT_LIST_DIR}/effect_token.hpp
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/effect_codegen_spirv.cpp
          ${CMAKE_CURRENT_LIST_DIR}/effect_expression.cpp
          ${CMAKE_CURRENT_LIST_DIR}/effect_lexer.cpp
          ${CMAKE_CURRENT_LIST_DIR}/effect_parser.cpp
          ${CMAKE_CURRENT_LIST_DIR}/effect_preprocessor.cpp
          ${CMAKE_CURRENT_LIST_DIR}/effect_symbol_table_intrinsics.inl
          ${CMAKE_CURRENT_LIST_DIR}/effect_symbol_table.cpp)

set_target_properties(vkBasalt_reshade PROPERTIES POSITION_INDEPENDENT_CODE
                                                  TRUE)

target_link_libraries(vkBasalt_reshade PUBLIC SPIRV-Headers::SPIRV-Headers)
