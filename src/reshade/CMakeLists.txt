cmake_minimum_required(VERSION 3.31)

add_library(vkbasalt_reshade OBJECT)

target_sources(
  vkbasalt_reshade
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

set_target_properties(vkbasalt_reshade PROPERTIES POSITION_INDEPENDENT_CODE
                                                  TRUE)

target_link_libraries(vkbasalt_reshade PUBLIC SPIRV-Headers::SPIRV-Headers)
