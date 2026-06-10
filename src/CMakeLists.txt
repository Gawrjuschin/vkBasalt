cmake_minimum_required(VERSION 3.31)

# ===============================================================================
# Subprojects
# ===============================================================================
add_subdirectory(shader/)
add_subdirectory(reshade/)
add_subdirectory(keyboard_input/)
add_subdirectory(logger/)

# ===============================================================================
# Main library (VkLayer)
# ===============================================================================
add_library(vkBasalt_vkBasalt SHARED)
add_library(vkBasalt::vkBasalt ALIAS vkBasalt_vkBasalt)

# Config with default paths based on CMake install variables
configure_file(${CMAKE_CURRENT_LIST_DIR}/config.cpp.in
               ${CMAKE_CURRENT_BINARY_DIR}/config.cpp @ONLY)

# Exports macro header
include(GenerateExportHeader)
generate_export_header(vkBasalt_vkBasalt BASE_NAME vkBasalt)

target_sources(
  vkBasalt_vkBasalt
  PRIVATE FILE_SET
          generated_headers # used in basalt.cpp
          TYPE
          HEADERS
          BASE_DIRS
          ${CMAKE_CURRENT_BINARY_DIR}
          FILES
          ${CMAKE_CURRENT_BINARY_DIR}/vkbasalt_export.h
  PRIVATE FILE_SET
          project_headers # used by generated config.cpp in
                          # CMAKE_CURRENT_BINARY_DIR
          TYPE
          HEADERS
          BASE_DIRS
          ${CMAKE_CURRENT_LIST_DIR}
          FILES
          ${CMAKE_CURRENT_LIST_DIR}/config.hpp
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/config.cpp.in # to display in IDE
          ${CMAKE_CURRENT_BINARY_DIR}/config.cpp
          ${CMAKE_CURRENT_LIST_DIR}/basalt.cpp
          ${CMAKE_CURRENT_LIST_DIR}/buffer.hpp
          ${CMAKE_CURRENT_LIST_DIR}/buffer.cpp
          ${CMAKE_CURRENT_LIST_DIR}/command_buffer.hpp
          ${CMAKE_CURRENT_LIST_DIR}/command_buffer.cpp
          ${CMAKE_CURRENT_LIST_DIR}/descriptor_set.hpp
          ${CMAKE_CURRENT_LIST_DIR}/descriptor_set.cpp
          ${CMAKE_CURRENT_LIST_DIR}/effect_cas.hpp
          ${CMAKE_CURRENT_LIST_DIR}/effect_cas.cpp
          ${CMAKE_CURRENT_LIST_DIR}/effect_deband.hpp
          ${CMAKE_CURRENT_LIST_DIR}/effect_deband.cpp
          ${CMAKE_CURRENT_LIST_DIR}/effect_dls.hpp
          ${CMAKE_CURRENT_LIST_DIR}/effect_dls.cpp
          ${CMAKE_CURRENT_LIST_DIR}/effect_fxaa.hpp
          ${CMAKE_CURRENT_LIST_DIR}/effect_fxaa.cpp
          ${CMAKE_CURRENT_LIST_DIR}/effect_lut.hpp
          ${CMAKE_CURRENT_LIST_DIR}/effect_lut.cpp
          ${CMAKE_CURRENT_LIST_DIR}/effect_reshade.hpp
          ${CMAKE_CURRENT_LIST_DIR}/effect_reshade.cpp
          ${CMAKE_CURRENT_LIST_DIR}/effect_simple.hpp
          ${CMAKE_CURRENT_LIST_DIR}/effect_simple.cpp
          ${CMAKE_CURRENT_LIST_DIR}/effect_smaa.hpp
          ${CMAKE_CURRENT_LIST_DIR}/effect_smaa.cpp
          ${CMAKE_CURRENT_LIST_DIR}/effect_transfer.hpp
          ${CMAKE_CURRENT_LIST_DIR}/effect_transfer.cpp
          ${CMAKE_CURRENT_LIST_DIR}/effect.hpp
          ${CMAKE_CURRENT_LIST_DIR}/fake_swapchain.hpp
          ${CMAKE_CURRENT_LIST_DIR}/fake_swapchain.cpp
          ${CMAKE_CURRENT_LIST_DIR}/format.hpp
          ${CMAKE_CURRENT_LIST_DIR}/format.cpp
          ${CMAKE_CURRENT_LIST_DIR}/framebuffer.hpp
          ${CMAKE_CURRENT_LIST_DIR}/framebuffer.cpp
          ${CMAKE_CURRENT_LIST_DIR}/graphics_pipeline.hpp
          ${CMAKE_CURRENT_LIST_DIR}/graphics_pipeline.cpp
          ${CMAKE_CURRENT_LIST_DIR}/image_view.hpp
          ${CMAKE_CURRENT_LIST_DIR}/image_view.cpp
          ${CMAKE_CURRENT_LIST_DIR}/image.hpp
          ${CMAKE_CURRENT_LIST_DIR}/image.cpp
          ${CMAKE_CURRENT_LIST_DIR}/logical_device.hpp
          ${CMAKE_CURRENT_LIST_DIR}/logical_swapchain.hpp
          ${CMAKE_CURRENT_LIST_DIR}/logical_swapchain.cpp
          ${CMAKE_CURRENT_LIST_DIR}/lut_cube.hpp
          ${CMAKE_CURRENT_LIST_DIR}/lut_cube.cpp
          ${CMAKE_CURRENT_LIST_DIR}/memory.hpp
          ${CMAKE_CURRENT_LIST_DIR}/memory.cpp
          ${CMAKE_CURRENT_LIST_DIR}/renderpass.hpp
          ${CMAKE_CURRENT_LIST_DIR}/renderpass.cpp
          ${CMAKE_CURRENT_LIST_DIR}/reshade_uniforms.hpp
          ${CMAKE_CURRENT_LIST_DIR}/reshade_uniforms.cpp
          ${CMAKE_CURRENT_LIST_DIR}/sampler.hpp
          ${CMAKE_CURRENT_LIST_DIR}/sampler.cpp
          ${CMAKE_CURRENT_LIST_DIR}/shader_sources.hpp
          ${CMAKE_CURRENT_LIST_DIR}/shader.hpp
          ${CMAKE_CURRENT_LIST_DIR}/shader.cpp
          ${CMAKE_CURRENT_LIST_DIR}/util.hpp
          ${CMAKE_CURRENT_LIST_DIR}/util.cpp
          ${CMAKE_CURRENT_LIST_DIR}/vkdispatch.hpp
          ${CMAKE_CURRENT_LIST_DIR}/vkdispatch.cpp
          ${CMAKE_CURRENT_LIST_DIR}/vkfuncs.hpp
          ${CMAKE_CURRENT_LIST_DIR}/vulkan_include.hpp)

set_target_properties(
  vkBasalt_vkBasalt
  PROPERTIES POSITION_INDEPENDENT_CODE TRUE # Enables PIC
             C_VISIBILITY_PRESET hidden # Hides symbols
             CXX_VISIBILITY_PRESET hidden # Hides symbols
             VISIBILITY_INLINES_HIDDEN ON # Hides inlined functions
             EXPORT_NAME vkBasalt # Required to use cmake target in namespace
             OUTPUT_NAME vkBasalt # Actual library name
)

target_link_libraries(
  vkBasalt_vkBasalt
  PRIVATE Vulkan::Headers vkBasalt_logger vkBasalt_keyboard_input
          vkBasalt_shader vkBasalt_reshade)
