#include "logical_swapchain.hpp"

#include <span>

#include <logger.hpp>

namespace vkBasalt
{
    void LogicalSwapchain::destroy()
    {
        if (imageCount > 0)
        {
            effects.clear();
            defaultTransfer.reset();

            pLogicalDevice->vkd.FreeCommandBuffers(
                pLogicalDevice->device, pLogicalDevice->commandPool, std::size(commandBuffersEffect), std::data(commandBuffersEffect));
            pLogicalDevice->vkd.FreeCommandBuffers(
                pLogicalDevice->device, pLogicalDevice->commandPool, std::size(commandBuffersNoEffect), std::data(commandBuffersNoEffect));
            Logger::debug("after free commandbuffer");

            pLogicalDevice->vkd.FreeMemory(pLogicalDevice->device, fakeImageMemory, nullptr);

            for (auto& fakeImage : fakeImages)
            {
                pLogicalDevice->vkd.DestroyImage(pLogicalDevice->device, fakeImage, nullptr);
            }

            for (auto& semaphore : std::span{semaphores}.subspan(0, imageCount))
            {
                pLogicalDevice->vkd.DestroySemaphore(pLogicalDevice->device, semaphore, nullptr);
            }
            Logger::debug("after DestroySemaphore");
        }
    }
} // namespace vkBasalt
