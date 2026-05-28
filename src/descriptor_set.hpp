#ifndef DESCRIPTOR_SET_HPP_INCLUDED
#define DESCRIPTOR_SET_HPP_INCLUDED

#include <vector>

#include "logical_device.hpp"

namespace vkBasalt
{
    VkDescriptorPool createDescriptorPool(LogicalDevice* pLogicalDevice, const std::vector<VkDescriptorPoolSize>& poolSizes);

    VkDescriptorSetLayout createUniformBufferDescriptorSetLayout(LogicalDevice* pLogicalDevice);

    VkDescriptorSet writeBufferDescriptorSet(LogicalDevice*        pLogicalDevice,
                                             VkDescriptorPool      descriptorPool,
                                             VkDescriptorSetLayout descriptorSetLayout,
                                             VkBuffer              buffer);

    VkDescriptorSetLayout createImageSamplerDescriptorSetLayout(LogicalDevice* pLogicalDevice, uint32_t count);

    std::vector<VkDescriptorSet> allocateAndWriteImageSamplerDescriptorSets(LogicalDevice*                        pLogicalDevice,
                                                                            VkDescriptorPool                      descriptorPool,
                                                                            VkDescriptorSetLayout                 descriptorSetLayout,
                                                                            std::vector<VkSampler>                samplers,
                                                                            std::vector<std::vector<VkImageView>> imageViewsVectors);
} // namespace vkBasalt

#endif // DESCRIPTOR_SET_HPP_INCLUDED
