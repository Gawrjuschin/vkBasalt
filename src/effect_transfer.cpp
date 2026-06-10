#include "effect_transfer.hpp"
#include "logical_device.hpp"
#include "config.hpp"

#include <cstdint>
#include <memory>
#include <span>

#include <vulkan/vulkan_core.h>

#include <logger.hpp>

namespace vkBasalt
{
    TransferEffect::TransferEffect(LogicalDevice*           pLogicalDevice,
                                   VkFormat                 format,
                                   VkExtent2D               imageExtent,
                                   std::span<const VkImage> inputImages,
                                   std::span<const VkImage> outputImages,
                                   Config*                  pConfig) :
        pLogicalDevice{pLogicalDevice}, inputImages(std::cbegin(inputImages), std::cend(inputImages)),
        outputImages(std::cbegin(outputImages), std::cend(outputImages)), imageExtent{imageExtent}, format{format}, pConfig{pConfig}
    {
    }

    void TransferEffect::applyEffect(uint32_t imageIndex, VkCommandBuffer commandBuffer)
    {
        if (std::size(inputImages) <= imageIndex)
        {
            Logger::err("imageIndex is out of range");
            return;
        }

        const VkImageCopy imageCopy{.srcSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1},
                                    .srcOffset      = {},
                                    .dstSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1},
                                    .dstOffset      = {},
                                    .extent         = {.width = imageExtent.width, .height = imageExtent.height, .depth = 1}};

        VkImageMemoryBarrier memoryBarrier{
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext               = nullptr,
            .srcAccessMask       = 0,
            .dstAccessMask       = VK_ACCESS_TRANSFER_READ_BIT,
            .oldLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = inputImages[imageIndex],
            .subresourceRange    = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}

        };
        pLogicalDevice->vkd.CmdPipelineBarrier(commandBuffer,
                                               VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                               VK_PIPELINE_STAGE_TRANSFER_BIT,
                                               0,
                                               0,
                                               nullptr,
                                               0,
                                               nullptr,
                                               1,
                                               std::addressof(memoryBarrier));

        memoryBarrier.image         = outputImages[imageIndex];
        memoryBarrier.oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
        memoryBarrier.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        pLogicalDevice->vkd.CmdPipelineBarrier(commandBuffer,
                                               VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                               VK_PIPELINE_STAGE_TRANSFER_BIT,
                                               0,
                                               0,
                                               nullptr,
                                               0,
                                               nullptr,
                                               1,
                                               std::addressof(memoryBarrier));

        pLogicalDevice->vkd.CmdCopyImage(commandBuffer,
                                         inputImages[imageIndex],
                                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                         outputImages[imageIndex],
                                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                         1,
                                         std::addressof(imageCopy));

        memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        memoryBarrier.dstAccessMask = 0;
        memoryBarrier.image         = outputImages[imageIndex];
        memoryBarrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        memoryBarrier.newLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        pLogicalDevice->vkd.CmdPipelineBarrier(commandBuffer,
                                               VK_PIPELINE_STAGE_TRANSFER_BIT,
                                               VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                               0,
                                               0,
                                               nullptr,
                                               0,
                                               nullptr,
                                               1,
                                               std::addressof(memoryBarrier));

        memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        memoryBarrier.image         = inputImages[imageIndex];
        memoryBarrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        memoryBarrier.newLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        pLogicalDevice->vkd.CmdPipelineBarrier(commandBuffer,
                                               VK_PIPELINE_STAGE_TRANSFER_BIT,
                                               VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                               0,
                                               0,
                                               nullptr,
                                               0,
                                               nullptr,
                                               1,
                                               std::addressof(memoryBarrier));
    }

    TransferEffect::~TransferEffect() = default;

} // namespace vkBasalt
