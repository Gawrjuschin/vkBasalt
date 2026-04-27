cmake_minimum_required(VERSION 3.31)

add_library(vkBasalt SHARED)

add_subdirectory(reshade/)
add_subdirectory(shader/)

target_sources(
  vkBasalt
  PRIVATE AreaTex.h
          basalt.cpp
          buffer.hpp
          buffer.cpp
          command_buffer.hpp
          command_buffer.cpp
          config.hpp
          config.cpp
          descriptor_set.hpp
          descriptor_set.cpp
          effect_cas.hpp
          effect_cas.cpp
          effect_deband.hpp
          effect_deband.cpp
          effect_dls.hpp
          effect_dls.cpp
          effect_fxaa.hpp
          effect_fxaa.cpp
          effect_lut.hpp
          effect_lut.cpp
          effect_reshade.hpp
          effect_reshade.cpp
          effect_simple.hpp
          effect_simple.cpp
          effect_smaa.hpp
          effect_smaa.cpp
          effect_transfer.hpp
          effect_transfer.cpp
          effect.hpp
          effect.cpp
          fake_swapchain.hpp
          fake_swapchain.cpp
          format.hpp
          format.cpp
          framebuffer.hpp
          framebuffer.cpp
          graphics_pipeline.hpp
          graphics_pipeline.cpp
          image_view.hpp
          image_view.cpp
          image.hpp
          image.hpp
          keyboard_input_x11.hpp
          keyboard_input_x11.cpp
          keyboard_input.hpp
          keyboard_input.cpp
          logger.hpp
          logger.cpp
          logical_device.hpp
          logical_swapchain.hpp
          logical_swapchain.cpp
          lut_cube.hpp
          lut_cube.cpp
          memory.hpp
          memory.cpp
          renderpass.hpp
          renderpass.cpp
          reshade_uniforms.hpp
          reshade_uniforms.cpp
          sampler.hpp
          sampler.cpp
          SearchTex.h
          shader_sources.hpp
          shader.hpp
          shader.cpp
          stb_image_dds.h
          stb_image_resize.h
          stb_image_resize.c
          stb_image.h
          stb_image.c
          util.hpp
          util.cpp
          vkdispatch.hpp
          vkdispatch.cpp
          vkfuncs.hpp
          vulkan_include.hpp)

target_link_libraries(vkBasalt PRIVATE vkBasalt_reshade vkBasalt_shaders)
