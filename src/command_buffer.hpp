#ifndef COMMAND_BUFFER_HPP_INCLUDED
#define COMMAND_BUFFER_HPP_INCLUDED

#include "effect.hpp"
#include "logical_device.hpp"

#include <cstdint>
#include <memory>
#include <vector>
#include <span>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    std::vector<VkCommandBuffer> allocateCommandBuffer(LogicalDevice* pLogicalDevice, uint32_t count);

    void writeCommandBuffers(LogicalDevice*                               pLogicalDevice,
                             std::span<std::unique_ptr<vkBasalt::Effect>> effects,
                             VkImage                                      depthImage,
                             VkImageView                                  depthImageView,
                             VkFormat                                     depthFormat,
                             std::span<VkCommandBuffer>                   commandBuffers);

    std::vector<VkSemaphore> createSemaphores(LogicalDevice* pLogicalDevice, uint32_t count);
} // namespace vkBasalt

#endif // COMMAND_BUFFER_HPP_INCLUDED
