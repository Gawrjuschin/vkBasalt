#include "image_view.hpp"
#include "logical_device.hpp"
#include "vulkan_include.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <span>

namespace vkBasalt
{
    VkImageView createImageView(LogicalDevice*     pLogicalDevice,
                                VkFormat           format,
                                const VkImage&     image,
                                VkImageViewType    viewType,
                                VkImageAspectFlags aspectMask,
                                uint32_t           mipLevels)
    {
        const VkImageViewCreateInfo imageViewCreateInfo{
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext            = nullptr,
            .flags            = 0,
            .image            = image,
            .viewType         = viewType,
            .format           = format,
            .components       = {.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                                 .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                                 .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                                 .a = VK_COMPONENT_SWIZZLE_IDENTITY},
            .subresourceRange = {.aspectMask = aspectMask, .baseMipLevel = 0, .levelCount = mipLevels, .baseArrayLayer = 0, .layerCount = 1}};

        VkImageView imageView{};
        const auto  result =
            pLogicalDevice->vkd.CreateImageView(pLogicalDevice->device, std::addressof(imageViewCreateInfo), nullptr, std::addressof(imageView));
        AssertVulkan(result);
        return imageView;
    }

    std::vector<VkImageView> createImageViews(LogicalDevice*           pLogicalDevice,
                                              VkFormat                 format,
                                              std::span<const VkImage> images,
                                              VkImageViewType          viewType,
                                              VkImageAspectFlags       aspectMask,
                                              uint32_t                 mipLevels)
    {
        VkImageViewCreateInfo imageViewCreateInfo{
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext            = nullptr,
            .flags            = 0,
            .image            = VK_NULL_HANDLE,
            .viewType         = viewType,
            .format           = format,
            .components       = {.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                                 .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                                 .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                                 .a = VK_COMPONENT_SWIZZLE_IDENTITY},
            .subresourceRange = {.aspectMask = aspectMask, .baseMipLevel = 0, .levelCount = mipLevels, .baseArrayLayer = 0, .layerCount = 1}};

        std::vector<VkImageView> imageViews(std::size(images));
        std::ranges::transform(images, std::begin(imageViews), [pLogicalDevice, &imageViewCreateInfo](const VkImage& image) -> VkImageView {
            VkImageView imageView{};
            imageViewCreateInfo.image = image;
            const auto result =
                pLogicalDevice->vkd.CreateImageView(pLogicalDevice->device, std::addressof(imageViewCreateInfo), nullptr, std::addressof(imageView));
            AssertVulkan(result);
            return imageView;
        });

        return imageViews;
    }

} // namespace vkBasalt
