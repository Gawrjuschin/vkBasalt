#ifndef LOGICAL_SWAPCHAIN_HPP_INCLUDED
#define LOGICAL_SWAPCHAIN_HPP_INCLUDED

#include "effect.hpp"
#include "logical_device.hpp"

#include <cstdint>
#include <vector>
#include <memory>

#include <vulkan/vulkan_core.h>

#include <logger.hpp>

namespace vkBasalt
{
    // for each swapchain, we have the Images and the other stuff we need to execute the compute shader
    struct LogicalSwapchain
    {
        LogicalDevice*                       pLogicalDevice;
        VkSwapchainCreateInfoKHR             swapchainCreateInfo;
        VkExtent2D                           imageExtent;
        VkFormat                             format;
        uint32_t                             imageCount;
        std::vector<VkImage>                 images;
        std::vector<VkImage>                 fakeImages;
        std::vector<VkCommandBuffer>         commandBuffersEffect;
        std::vector<VkCommandBuffer>         commandBuffersNoEffect;
        std::vector<VkSemaphore>             semaphores;
        std::vector<std::unique_ptr<Effect>> effects;
        std::unique_ptr<Effect>              defaultTransfer;
        VkDeviceMemory                       fakeImageMemory;
    };

    inline void Destroy(LogicalSwapchain& logicalSwapchain)
    {
        if (logicalSwapchain.imageCount > 0)
        {
            logicalSwapchain.effects.clear();
            logicalSwapchain.defaultTransfer.reset();

            logicalSwapchain.pLogicalDevice->vkd.FreeCommandBuffers(logicalSwapchain.pLogicalDevice->device,
                                                                    logicalSwapchain.pLogicalDevice->commandPool,
                                                                    std::size(logicalSwapchain.commandBuffersEffect),
                                                                    std::data(logicalSwapchain.commandBuffersEffect));
            logicalSwapchain.pLogicalDevice->vkd.FreeCommandBuffers(logicalSwapchain.pLogicalDevice->device,
                                                                    logicalSwapchain.pLogicalDevice->commandPool,
                                                                    std::size(logicalSwapchain.commandBuffersNoEffect),
                                                                    std::data(logicalSwapchain.commandBuffersNoEffect));
            Logger::debug("after free commandbuffer");

            logicalSwapchain.pLogicalDevice->vkd.FreeMemory(logicalSwapchain.pLogicalDevice->device, logicalSwapchain.fakeImageMemory, nullptr);

            for (auto& fakeImage : logicalSwapchain.fakeImages)
            {
                logicalSwapchain.pLogicalDevice->vkd.DestroyImage(logicalSwapchain.pLogicalDevice->device, fakeImage, nullptr);
            }

            for (auto& semaphore : std::span{logicalSwapchain.semaphores}.subspan(0, logicalSwapchain.imageCount))
            {
                logicalSwapchain.pLogicalDevice->vkd.DestroySemaphore(logicalSwapchain.pLogicalDevice->device, semaphore, nullptr);
            }

            Logger::debug("after DestroySemaphore");
        }
    }

} // namespace vkBasalt

#endif // LOGICAL_SWAPCHAIN_HPP_INCLUDED
