#include "image.hpp"
#include "logical_device.hpp"
#include "memory.hpp"
#include "buffer.hpp"
#include "format.hpp"
#include "vulkan_include.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <ranges>
#include <span>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    VkImage createImage(LogicalDevice*        pLogicalDevice,
                        VkExtent3D            extent,
                        VkFormat              format,
                        VkImageUsageFlags     usage,
                        VkMemoryPropertyFlags properties,
                        VkDeviceMemory&       imageMemory,
                        uint32_t              mipLevels)
    {
        VkImage image{};

        const auto unormFormat = isSRGB(format) ? convertToUNORM(format) : format;
        const auto srgbFormat  = isSRGB(format) ? format : convertToSRGB(format);

        const std::array formats{unormFormat, srgbFormat};

        const VkImageFormatListCreateInfoKHR imageFormatListCreateInfo{.sType           = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR,
                                                                       .pNext           = nullptr,
                                                                       .viewFormatCount = std::size(formats),
                                                                       .pViewFormats    = std::data(formats)};

        const VkImageCreateInfo imageCreateInfo{
            .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext                 = (unormFormat == srgbFormat) ? nullptr : std::addressof(imageFormatListCreateInfo),
            .flags                 = static_cast<VkImageCreateFlags>((unormFormat == srgbFormat) ? 0 : VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT),
            .imageType             = (extent.depth == 1) ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D,
            .format                = format,
            .extent                = extent,
            .mipLevels             = mipLevels,
            .arrayLayers           = 1,
            .samples               = VK_SAMPLE_COUNT_1_BIT,
            .tiling                = VK_IMAGE_TILING_OPTIMAL,
            .usage                 = usage,
            .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,       // Don't care
            .pQueueFamilyIndices   = nullptr, // Don't care
            .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED};

        {
            const auto result =
                pLogicalDevice->vkd.CreateImage(pLogicalDevice->device, std::addressof(imageCreateInfo), nullptr, std::addressof(image));
            AssertVulkan(result);
        }

        // Allocate a bunch of memory for all images at one
        VkMemoryRequirements memoryRequirements;
        pLogicalDevice->vkd.GetImageMemoryRequirements(pLogicalDevice->device, image, std::addressof(memoryRequirements));

        if (memoryRequirements.size % memoryRequirements.alignment != 0)
        {
            memoryRequirements.size = (memoryRequirements.size / memoryRequirements.alignment + 1) * memoryRequirements.alignment;
        }

        const VkMemoryAllocateInfo memoryAllocateInfo{.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                                      .pNext          = nullptr,
                                                      .allocationSize = memoryRequirements.size * 1U,
                                                      .memoryTypeIndex =
                                                          findMemoryTypeIndex(pLogicalDevice, memoryRequirements.memoryTypeBits, properties)};

        const auto result =
            pLogicalDevice->vkd.AllocateMemory(pLogicalDevice->device, std::addressof(memoryAllocateInfo), nullptr, std::addressof(imageMemory));
        AssertVulkan(result);

        {
            const auto result = pLogicalDevice->vkd.BindImageMemory(pLogicalDevice->device, image, imageMemory, memoryRequirements.size * 0U);
            AssertVulkan(result);
        }
        return image;
    }

    std::vector<VkImage> createImages(LogicalDevice*        pLogicalDevice,
                                      uint32_t              count,
                                      VkExtent3D            extent,
                                      VkFormat              format,
                                      VkImageUsageFlags     usage,
                                      VkMemoryPropertyFlags properties,
                                      VkDeviceMemory&       imageMemory,
                                      uint32_t              mipLevels)
    {
        std::vector<VkImage> images(count);

        const auto unormFormat = isSRGB(format) ? convertToUNORM(format) : format;
        const auto srgbFormat  = isSRGB(format) ? format : convertToSRGB(format);

        const std::array formats{unormFormat, srgbFormat};

        const VkImageFormatListCreateInfoKHR imageFormatListCreateInfo{.sType           = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR,
                                                                       .pNext           = nullptr,
                                                                       .viewFormatCount = std::size(formats),
                                                                       .pViewFormats    = std::data(formats)};

        const VkImageCreateInfo imageCreateInfo{
            .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext                 = (unormFormat == srgbFormat) ? nullptr : std::addressof(imageFormatListCreateInfo),
            .flags                 = static_cast<VkImageCreateFlags>((unormFormat == srgbFormat) ? 0 : VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT),
            .imageType             = (extent.depth == 1) ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D,
            .format                = format,
            .extent                = extent,
            .mipLevels             = mipLevels,
            .arrayLayers           = 1,
            .samples               = VK_SAMPLE_COUNT_1_BIT,
            .tiling                = VK_IMAGE_TILING_OPTIMAL,
            .usage                 = usage,
            .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,       // Don't care
            .pQueueFamilyIndices   = nullptr, // Don't care
            .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED};

        for (auto& image : images)
        {
            const auto result =
                pLogicalDevice->vkd.CreateImage(pLogicalDevice->device, std::addressof(imageCreateInfo), nullptr, std::addressof(image));
            AssertVulkan(result);
        }
        // Allocate a bunch of memory for all images at one
        VkMemoryRequirements memoryRequirements;
        pLogicalDevice->vkd.GetImageMemoryRequirements(pLogicalDevice->device, images.front(), std::addressof(memoryRequirements));

        if (memoryRequirements.size % memoryRequirements.alignment != 0)
        {
            memoryRequirements.size = (memoryRequirements.size / memoryRequirements.alignment + 1) * memoryRequirements.alignment;
        }

        const VkMemoryAllocateInfo memoryAllocateInfo{.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                                      .pNext          = nullptr,
                                                      .allocationSize = memoryRequirements.size * count,
                                                      .memoryTypeIndex =
                                                          findMemoryTypeIndex(pLogicalDevice, memoryRequirements.memoryTypeBits, properties)};

        const auto result =
            pLogicalDevice->vkd.AllocateMemory(pLogicalDevice->device, std::addressof(memoryAllocateInfo), nullptr, std::addressof(imageMemory));
        AssertVulkan(result);

        for (auto [idx, image] : images | std::views::enumerate)
        {
            const auto result = pLogicalDevice->vkd.BindImageMemory(pLogicalDevice->device, image, imageMemory, memoryRequirements.size * idx);
            AssertVulkan(result);
        }
        return images;
    }

    void
    uploadToImage(LogicalDevice* pLogicalDevice, VkImage image, VkExtent3D extent, uint32_t size, const unsigned char* writeData, uint32_t mipLevels)
    {

        VkBuffer       stagingBuffer{};
        VkDeviceMemory stagingMemory{};

        createBuffer(pLogicalDevice,
                     size,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer,
                     stagingMemory);

        void*      data{};
        const auto result = pLogicalDevice->vkd.MapMemory(pLogicalDevice->device, stagingMemory, 0, size, 0, std::addressof(data));
        AssertVulkan(result);

        std::memcpy(data, writeData, size);
        pLogicalDevice->vkd.UnmapMemory(pLogicalDevice->device, stagingMemory);

        const VkCommandBufferAllocateInfo allocInfo{.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                                    .commandPool        = pLogicalDevice->commandPool,
                                                    .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                    .commandBufferCount = 1};

        VkCommandBuffer commandBuffer{};
        pLogicalDevice->vkd.AllocateCommandBuffers(pLogicalDevice->device, std::addressof(allocInfo), std::addressof(commandBuffer));
        // initialize dispatch table for commandBuffer since it is a dispatchable object
        initializeDispatchTable(commandBuffer, pLogicalDevice->device);

        const VkCommandBufferBeginInfo beginInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                                 .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

        pLogicalDevice->vkd.BeginCommandBuffer(commandBuffer, std::addressof(beginInfo));

        VkImageMemoryBarrier memoryBarrier{
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext               = nullptr,
            .srcAccessMask       = 0,
            .dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = image,
            .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}};

        pLogicalDevice->vkd.CmdPipelineBarrier(commandBuffer,
                                               VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                               VK_PIPELINE_STAGE_TRANSFER_BIT,
                                               0,
                                               0,
                                               nullptr,
                                               0,
                                               nullptr,
                                               1,
                                               std::addressof(memoryBarrier));

        const VkBufferImageCopy region{
            .bufferOffset      = 0,
            .bufferRowLength   = 0,
            .bufferImageHeight = 0,
            .imageSubresource  = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
            .imageOffset       = {.x = 0, .y = 0, .z = 0},
            .imageExtent       = extent};

        pLogicalDevice->vkd.CmdCopyBufferToImage(
            commandBuffer, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, std::addressof(region));

        memoryBarrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        memoryBarrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        pLogicalDevice->vkd.CmdPipelineBarrier(commandBuffer,
                                               VK_PIPELINE_STAGE_TRANSFER_BIT,
                                               VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                               0,
                                               0,
                                               nullptr,
                                               0,
                                               nullptr,
                                               1,
                                               std::addressof(memoryBarrier));

        generateMipMaps(pLogicalDevice, commandBuffer, image, extent, mipLevels);

        pLogicalDevice->vkd.EndCommandBuffer(commandBuffer);

        const VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = std::addressof(commandBuffer)};

        pLogicalDevice->vkd.QueueSubmit(pLogicalDevice->queue, 1, std::addressof(submitInfo), VK_NULL_HANDLE);
        pLogicalDevice->vkd.QueueWaitIdle(pLogicalDevice->queue);

        pLogicalDevice->vkd.FreeCommandBuffers(pLogicalDevice->device, pLogicalDevice->commandPool, 1, std::addressof(commandBuffer));
        pLogicalDevice->vkd.FreeMemory(pLogicalDevice->device, stagingMemory, nullptr);
        pLogicalDevice->vkd.DestroyBuffer(pLogicalDevice->device, stagingBuffer, nullptr);
    }

    void changeImageLayout(LogicalDevice* pLogicalDevice, std::span<const VkImage> images, uint32_t mipLevels)
    {
        VkCommandBufferAllocateInfo allocInfo = {.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                                 .commandPool        = pLogicalDevice->commandPool,
                                                 .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                 .commandBufferCount = 1};

        VkCommandBuffer commandBuffer{};
        pLogicalDevice->vkd.AllocateCommandBuffers(pLogicalDevice->device, std::addressof(allocInfo), std::addressof(commandBuffer));
        // initialize dispatch table for commandBuffer since it is a dispatchable object
        initializeDispatchTable(commandBuffer, pLogicalDevice->device);

        VkCommandBufferBeginInfo beginInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                           .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

        pLogicalDevice->vkd.BeginCommandBuffer(commandBuffer, std::addressof(beginInfo));

        VkImageMemoryBarrier memoryBarrier{
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext               = nullptr,
            .srcAccessMask       = 0,
            .dstAccessMask       = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = {},
            .subresourceRange    = {
                   .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = mipLevels, .baseArrayLayer = 0, .layerCount = 1}};

        for (const auto& image : images)
        {
            memoryBarrier.image = image;
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

        pLogicalDevice->vkd.EndCommandBuffer(commandBuffer);

        const VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = std::addressof(commandBuffer)};

        pLogicalDevice->vkd.QueueSubmit(pLogicalDevice->queue, 1, std::addressof(submitInfo), VK_NULL_HANDLE);
        pLogicalDevice->vkd.QueueWaitIdle(pLogicalDevice->queue);

        pLogicalDevice->vkd.FreeCommandBuffers(pLogicalDevice->device, pLogicalDevice->commandPool, 1, std::addressof(commandBuffer));
    }

    void generateMipMaps(LogicalDevice* pLogicalDevice, VkCommandBuffer commandBuffer, VkImage image, VkExtent3D extent, uint32_t mipLevels)
    {
        if (mipLevels < 2)
        {
            return;
        }

        auto width  = static_cast<int32_t>(extent.width);
        auto height = static_cast<int32_t>(extent.height);
        auto depth  = static_cast<int32_t>(extent.depth);

        VkImageMemoryBarrier memoryBarrier{
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext               = nullptr,
            .srcAccessMask       = 0,
            .dstAccessMask       = VK_ACCESS_TRANSFER_READ_BIT,
            .oldLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = image,
            .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}};

        for (uint32_t i = 1; i < mipLevels; i++)
        {
            const VkImageBlit imageBlit{
                .srcSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = i - 1, .baseArrayLayer = 0, .layerCount = 1},
                .srcOffsets     = {{.x = 0, .y = 0, .z = 0}, {.x = width, .y = height, .z = depth}},
                .dstSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = i, .baseArrayLayer = 0, .layerCount = 1},
                .dstOffsets     = {{.x = 0, .y = 0, .z = 0},
                                   {.x = ((width == 1) ? 1 : width /= 2), .y = ((height == 1) ? 1 : height /= 2), .z = ((depth == 1) ? 1 : depth /= 2)}}};

            memoryBarrier.subresourceRange.baseMipLevel = i - 1;
            memoryBarrier.oldLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            memoryBarrier.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            memoryBarrier.srcAccessMask = 0;
            memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            pLogicalDevice->vkd.CmdPipelineBarrier(commandBuffer,
                                                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                   VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                   0,
                                                   0,
                                                   nullptr,
                                                   0,
                                                   nullptr,
                                                   1,
                                                   std::addressof(memoryBarrier));

            memoryBarrier.subresourceRange.baseMipLevel = i;

            memoryBarrier.oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
            memoryBarrier.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            memoryBarrier.srcAccessMask = 0;
            memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            pLogicalDevice->vkd.CmdPipelineBarrier(commandBuffer,
                                                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                   VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                   0,
                                                   0,
                                                   nullptr,
                                                   0,
                                                   nullptr,
                                                   1,
                                                   std::addressof(memoryBarrier));

            pLogicalDevice->vkd.CmdBlitImage(commandBuffer,
                                             image,
                                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                             image,
                                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                             1,
                                             std::addressof(imageBlit),
                                             VK_FILTER_LINEAR);

            memoryBarrier.subresourceRange.baseMipLevel = i - 1;

            memoryBarrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            memoryBarrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            pLogicalDevice->vkd.CmdPipelineBarrier(commandBuffer,
                                                   VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                   VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                                                   0,
                                                   0,
                                                   nullptr,
                                                   0,
                                                   nullptr,
                                                   1,
                                                   std::addressof(memoryBarrier));

            memoryBarrier.subresourceRange.baseMipLevel = i;

            memoryBarrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            memoryBarrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            pLogicalDevice->vkd.CmdPipelineBarrier(commandBuffer,
                                                   VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                   VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                                                   0,
                                                   0,
                                                   nullptr,
                                                   0,
                                                   nullptr,
                                                   1,
                                                   std::addressof(memoryBarrier));
        }
    }
} // namespace vkBasalt
