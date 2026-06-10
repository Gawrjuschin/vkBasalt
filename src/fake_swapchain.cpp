#include "fake_swapchain.hpp"
#include "logical_device.hpp"
#include "logical_swapchain.hpp"
#include "memory.hpp"
#include "format.hpp"
#include "vulkan_include.hpp"

#include <array>
#include <cstdint>
#include <iterator>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <vector>

#include <vulkan/vulkan_core.h>

#include <logger.hpp>

namespace vkBasalt
{
    std::vector<VkImage> createFakeSwapchainImages(LogicalDevice& logicalDevice, LogicalSwapchain& logicalSwapchain, uint32_t effectsCount)
    {
        // create 1 more set of images when we can't use the swapchain it self
        logicalSwapchain.fakeImages.resize(logicalDevice.supportsMutableFormat ? logicalSwapchain.imageCount * effectsCount
                                                                               : logicalSwapchain.imageCount * (effectsCount + 1U));

        const auto srgbFormat  = isSRGB(logicalSwapchain.swapchainCreateInfo.imageFormat)
                                     ? logicalSwapchain.swapchainCreateInfo.imageFormat
                                     : convertToSRGB(logicalSwapchain.swapchainCreateInfo.imageFormat);
        const auto unormFormat = isSRGB(logicalSwapchain.swapchainCreateInfo.imageFormat)
                                     ? convertToUNORM(logicalSwapchain.swapchainCreateInfo.imageFormat)
                                     : logicalSwapchain.swapchainCreateInfo.imageFormat;

        const std::array formats{unormFormat, srgbFormat};

        VkImageFormatListCreateInfoKHR imageFormatListCreateInfo{.sType           = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR,
                                                                 .pNext           = nullptr,
                                                                 .viewFormatCount = std::size(formats),
                                                                 .pViewFormats    = std::data(formats)};

        const VkImageCreateInfo imageCreateInfo{
            .sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext       = (unormFormat == srgbFormat) ? nullptr : std::addressof(imageFormatListCreateInfo),
            .flags       = static_cast<VkImageCreateFlags>((unormFormat == srgbFormat) ? 0 : VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT),
            .imageType   = VK_IMAGE_TYPE_2D,
            .format      = logicalSwapchain.swapchainCreateInfo.imageFormat,
            .extent      = {.width  = logicalSwapchain.swapchainCreateInfo.imageExtent.width,
                            .height = logicalSwapchain.swapchainCreateInfo.imageExtent.height,
                            .depth  = 1},
            .mipLevels   = 1,
            .arrayLayers = logicalSwapchain.swapchainCreateInfo.imageArrayLayers,
            .samples     = VK_SAMPLE_COUNT_1_BIT,
            .tiling      = VK_IMAGE_TILING_OPTIMAL,
            .usage       = logicalSwapchain.swapchainCreateInfo.imageUsage | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                     | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, // TODO what usage do we need?
            .sharingMode           = logicalSwapchain.swapchainCreateInfo.imageSharingMode,
            .queueFamilyIndexCount = logicalSwapchain.swapchainCreateInfo.queueFamilyIndexCount,
            .pQueueFamilyIndices   = logicalSwapchain.swapchainCreateInfo.pQueueFamilyIndices,
            .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED};

        for (auto& fakeImage : logicalSwapchain.fakeImages)
        {
            const auto result =
                logicalDevice.vkd.CreateImage(logicalDevice.device, std::addressof(imageCreateInfo), nullptr, std::addressof(fakeImage));
            AssertVulkan(result);
        }

        // Allocate a bunch of memory for all images at one
        VkMemoryRequirements memoryRequirements;
        logicalDevice.vkd.GetImageMemoryRequirements(logicalDevice.device, logicalSwapchain.fakeImages.front(), std::addressof(memoryRequirements));

        Logger::debug("fake image size: " + std::to_string(memoryRequirements.size));
        Logger::debug("fake image alignment: " + std::to_string(memoryRequirements.alignment));

        if (memoryRequirements.size % memoryRequirements.alignment != 0)
        {
            memoryRequirements.size = ((memoryRequirements.size / memoryRequirements.alignment) + 1) * memoryRequirements.alignment;
        }

        const VkMemoryAllocateInfo memoryAllocateInfo{
            .sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext          = nullptr,
            .allocationSize = memoryRequirements.size * std::size(logicalSwapchain.fakeImages),
            .memoryTypeIndex =
                findMemoryTypeIndex(std::addressof(logicalDevice), memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

        const auto result = logicalDevice.vkd.AllocateMemory(
            logicalDevice.device, std::addressof(memoryAllocateInfo), nullptr, std::addressof(logicalSwapchain.fakeImageMemory));
        AssertVulkan(result);

        for (auto [idx, fakeImage] : std::views::enumerate(logicalSwapchain.fakeImages))
        {
            const auto memoryOffset{memoryRequirements.size * idx};
            const auto result = logicalDevice.vkd.BindImageMemory(logicalDevice.device, fakeImage, logicalSwapchain.fakeImageMemory, memoryOffset);
            AssertVulkan(result);
        }

        // NOTICE: LogicalSwapchain frees all fakeImages that cause double free if we just write logicalSwapchain.images after fake ones.
        std::vector<VkImage> fakeImages = logicalSwapchain.fakeImages;
        if (logicalDevice.supportsMutableFormat)
        {
            fakeImages.insert(std::end(fakeImages), std::cbegin(logicalSwapchain.images), std::cend(logicalSwapchain.images));
        }

        return fakeImages;
    }
} // namespace vkBasalt
