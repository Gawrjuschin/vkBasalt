cmake_minimum_required(VERSION 3.31)

add_subdirectory(reshade/)
add_subdirectory(shader/)

# TODO: better place
add_library(vkBasalt_X11input OBJECT)
target_sources(
  vkBasalt_X11input
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/keyboard_input_x11.cpp
  PUBLIC FILE_SET HEADERS FILES
         ${CMAKE_CURRENT_LIST_DIR}/keyboard_input_x11.hpp)
target_link_libraries(vkBasalt_X11input PUBLIC X11::X11)

set(VKBASALT_SYSCONFDIR "${CMAKE_INSTALL_SYSCONFDIR}")
set(VKBASALT_DATADIR "${CMAKE_INSTALL_DATADIR}")
configure_file(${CMAKE_CURRENT_LIST_DIR}/config.cpp.in
               ${CMAKE_CURRENT_BINARY_DIR}/config.cpp @ONLY)

add_library(vkBasalt SHARED)

target_sources(
  vkBasalt
  PRIVATE FILE_SET HEADERS FILES ${CMAKE_CURRENT_LIST_DIR}/config.hpp
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/config.cpp.in
          ${CMAKE_CURRENT_BINARY_DIR}/config.cpp
          ${CMAKE_CURRENT_LIST_DIR}/AreaTex.h
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
          ${CMAKE_CURRENT_LIST_DIR}/effect.cpp
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
          ${CMAKE_CURRENT_LIST_DIR}/image.hpp
          ${CMAKE_CURRENT_LIST_DIR}/keyboard_input.hpp
          ${CMAKE_CURRENT_LIST_DIR}/keyboard_input.cpp
          ${CMAKE_CURRENT_LIST_DIR}/logger.hpp
          ${CMAKE_CURRENT_LIST_DIR}/logger.cpp
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
          ${CMAKE_CURRENT_LIST_DIR}/SearchTex.h
          ${CMAKE_CURRENT_LIST_DIR}/shader_sources.hpp
          ${CMAKE_CURRENT_LIST_DIR}/shader.hpp
          ${CMAKE_CURRENT_LIST_DIR}/shader.cpp
          ${CMAKE_CURRENT_LIST_DIR}/stb_image_dds.h
          ${CMAKE_CURRENT_LIST_DIR}/stb_image_resize.h
          ${CMAKE_CURRENT_LIST_DIR}/stb_image_resize.c
          ${CMAKE_CURRENT_LIST_DIR}/stb_image.h
          ${CMAKE_CURRENT_LIST_DIR}/stb_image.c
          ${CMAKE_CURRENT_LIST_DIR}/util.hpp
          ${CMAKE_CURRENT_LIST_DIR}/util.cpp
          ${CMAKE_CURRENT_LIST_DIR}/vkdispatch.hpp
          ${CMAKE_CURRENT_LIST_DIR}/vkdispatch.cpp
          ${CMAKE_CURRENT_LIST_DIR}/vkfuncs.hpp
          ${CMAKE_CURRENT_LIST_DIR}/vulkan_include.hpp)

set_target_properties(vkBasalt PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

target_link_libraries(vkBasalt PRIVATE vkBasalt_reshade vkBasalt_shader)
