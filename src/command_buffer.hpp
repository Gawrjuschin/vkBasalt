#ifndef COMMAND_BUFFER_HPP_INCLUDED
#define COMMAND_BUFFER_HPP_INCLUDED

#include "effect.hpp"
#include "logical_device.hpp"

#include <vector>
#include <memory>

namespace vkBasalt
{
    std::vector<VkCommandBuffer> allocateCommandBuffer(LogicalDevice* pLogicalDevice, uint32_t count);

    void writeCommandBuffers(LogicalDevice*                                 pLogicalDevice,
                             std::vector<std::shared_ptr<vkBasalt::Effect>> effects,
                             VkImage                                        depthImage,
                             VkImageView                                    depthImageView,
                             VkFormat                                       depthFormat,
                             std::vector<VkCommandBuffer>                   commandBuffers);

    std::vector<VkSemaphore> createSemaphores(LogicalDevice* pLogicalDevice, uint32_t count);
} // namespace vkBasalt

#endif // COMMAND_BUFFER_HPP_INCLUDED
