#ifndef COMMAND_BUFFER_HPP_INCLUDED
#define COMMAND_BUFFER_HPP_INCLUDED

#include "effect.hpp"
#include "format.hpp"
#include "logical_device.hpp"
#include "util.hpp"
#include "vulkan_include.hpp"

#include <cstdint>
#include <memory>
#include <vector>
#include <span>
#include <ranges>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    inline std::vector<VkCommandBuffer> allocateCommandBuffer(LogicalDevice* pLogicalDevice, uint32_t count)
    {
        std::vector<VkCommandBuffer> commandBuffers(count);

        const VkCommandBufferAllocateInfo allocInfo{.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                                    .pNext              = nullptr,
                                                    .commandPool        = pLogicalDevice->commandPool,
                                                    .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                    .commandBufferCount = count};

        const VkResult result =
            pLogicalDevice->vkd.AllocateCommandBuffers(pLogicalDevice->device, std::addressof(allocInfo), std::data(commandBuffers));
        AssertVulkan(result);
        for (auto& commandBuffer : commandBuffers)
        {
            // initialize dispatch tables for commandBuffers since the are dispatchable objects
            initializeDispatchTable(commandBuffer, pLogicalDevice->device);
        }

        return commandBuffers;
    }

    inline void writeCommandBuffers(LogicalDevice*                               pLogicalDevice,
                                    std::span<std::unique_ptr<vkBasalt::Effect>> effects,
                                    VkImage                                      depthImage,
                                    VkImageView                                  depthImageView,
                                    VkFormat                                     depthFormat,
                                    std::span<VkCommandBuffer>                   commandBuffers)
    {
        constexpr static VkCommandBufferBeginInfo beginInfo{.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                                            .pNext            = nullptr,
                                                            .flags            = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
                                                            .pInheritanceInfo = nullptr};

        for (auto& effect : effects)
        {
            effect->useDepthImage(depthImageView);
        }

        for (auto [idx, commandBuffer] : commandBuffers | std::views::enumerate)
        {

            VkResult result = pLogicalDevice->vkd.BeginCommandBuffer(commandBuffer, std::addressof(beginInfo));
            AssertVulkan(result);

            VkImageMemoryBarrier memoryBarrier{
                .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .pNext               = nullptr,
                .srcAccessMask       = 0,
                .dstAccessMask       = VK_ACCESS_SHADER_READ_BIT,
                .oldLayout           = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                .newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = depthImage,
                .subresourceRange    = {.aspectMask     = static_cast<VkImageAspectFlags>(isStencilFormat(depthFormat)
                                                                                       ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
                                                                                       : VK_IMAGE_ASPECT_DEPTH_BIT),
                                        .baseMipLevel   = 0,
                                        .levelCount     = 1,
                                        .baseArrayLayer = 0,
                                        .layerCount     = 1}};

            if (depthImageView != nullptr)
            {
                pLogicalDevice->vkd.CmdPipelineBarrier(commandBuffer,
                                                       VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                       VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                                       0,
                                                       0,
                                                       nullptr,
                                                       0,
                                                       nullptr,
                                                       1,
                                                       std::addressof(memoryBarrier));
            }

            for (auto& effect : effects)
            {
                Logger::debug("before applying effect " + convertToString(effect));
                effect->applyEffect(idx, commandBuffer);
            }

            if (depthImageView != nullptr)
            {
                memoryBarrier.oldLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                memoryBarrier.newLayout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                memoryBarrier.dstAccessMask = 0;

                pLogicalDevice->vkd.CmdPipelineBarrier(commandBuffer,
                                                       VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                       VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                                       0,
                                                       0,
                                                       nullptr,
                                                       0,
                                                       nullptr,
                                                       1,
                                                       std::addressof(memoryBarrier));
            }

            result = pLogicalDevice->vkd.EndCommandBuffer(commandBuffer); // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
            AssertVulkan(result);
        }
    }

    inline std::vector<VkSemaphore> createSemaphores(LogicalDevice* pLogicalDevice, uint32_t count)
    {
        std::vector<VkSemaphore>               semaphores(count);
        constexpr static VkSemaphoreCreateInfo info{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = nullptr, .flags = 0};

        for (auto& semaphore : semaphores)
        {
            pLogicalDevice->vkd.CreateSemaphore(pLogicalDevice->device, std::addressof(info), nullptr, std::addressof(semaphore));
        }
        return semaphores;
    }
} // namespace vkBasalt

#endif // COMMAND_BUFFER_HPP_INCLUDED
