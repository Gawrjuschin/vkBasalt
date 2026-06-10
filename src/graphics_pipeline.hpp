#ifndef GRAPHICS_PIPELINE_HPP_INCLUDED
#define GRAPHICS_PIPELINE_HPP_INCLUDED

#include "logical_device.hpp"

#include <span>
#include <string>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    VkPipelineLayout createGraphicsPipelineLayout(LogicalDevice* pLogicalDevice, std::span<const VkDescriptorSetLayout> descriptorSetLayouts);

    VkPipeline createGraphicsPipeline(LogicalDevice*              pLogicalDevice,
                                      VkShaderModule              vertexModule,
                                      const VkSpecializationInfo* vertexSpecializationInfo,
                                      const std::string&          vertexEntryPoint,
                                      VkShaderModule              fragmentModule,
                                      const VkSpecializationInfo* fragmentSpecializationInfo,
                                      const std::string&          fragmentEntryPoint,
                                      VkExtent2D                  extent,
                                      VkRenderPass                renderPass,
                                      VkPipelineLayout            pipelineLayout,
                                      bool                        flip = false);

} // namespace vkBasalt

#endif // GRAPHICS_PIPELINE_HPP_INCLUDED
