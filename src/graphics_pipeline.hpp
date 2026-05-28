#ifndef GRAPHICS_PIPELINE_HPP_INCLUDED
#define GRAPHICS_PIPELINE_HPP_INCLUDED

#include "logical_device.hpp"

#include <vector>
#include <string>

namespace vkBasalt
{
    VkPipelineLayout createGraphicsPipelineLayout(LogicalDevice* pLogicalDevice, std::vector<VkDescriptorSetLayout> descriptorSetLayouts);

    VkPipeline createGraphicsPipeline(LogicalDevice*        pLogicalDevice,
                                      VkShaderModule        vertexModule,
                                      VkSpecializationInfo* vertexSpecializationInfo,
                                      std::string           vertexEntryPoint,
                                      VkShaderModule        fragmentModule,
                                      VkSpecializationInfo* fragmentSpecializationInfo,
                                      std::string           fragmentEntryPoint,
                                      VkExtent2D            extent,
                                      VkRenderPass          renderPass,
                                      VkPipelineLayout      pipelineLayout,
                                      bool                  flip = false);

} // namespace vkBasalt

#endif // GRAPHICS_PIPELINE_HPP_INCLUDED
