cmake_minimum_required(VERSION 3.31)

add_library(vkBasalt_reshade OBJECT)

target_sources(
  vkBasalt_reshade
  PUBLIC FILE_SET
         HEADERS
         FILES
         effect_codegen.hpp
         effect_expression.hpp
         effect_lexer.hpp
         effect_module.hpp
         effect_parser.hpp
         effect_preprocessor.hpp
         effect_symbol_table.hpp
         effect_token.hpp
  PRIVATE effect_codegen_spirv.cpp
          effect_expression.cpp
          effect_lexer.cpp
          effect_parser.cpp
          effect_preprocessor.cpp
          effect_symbol_table_intrinsics.inl
          effect_symbol_table.cpp
          GLSL.std.450.h
          spirv.hpp)
