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
    std::vector<VkImageView> createImageViews(LogicalDevice*           pLogicalDevice,
                                              VkFormat                 format,
                                              std::span<const VkImage> images,
                                              VkImageViewType          viewType,
                                              VkImageAspectFlags       aspectMask,
                                              uint32_t                 mipLevels)
    {

        VkImageViewCreateInfo imageViewCreateInfo;

        imageViewCreateInfo.sType        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext        = nullptr;
        imageViewCreateInfo.flags        = 0;
        imageViewCreateInfo.image        = VK_NULL_HANDLE;
        imageViewCreateInfo.viewType     = viewType;
        imageViewCreateInfo.format       = format;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        imageViewCreateInfo.subresourceRange.aspectMask     = aspectMask;
        imageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
        imageViewCreateInfo.subresourceRange.levelCount     = mipLevels;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount     = 1;

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
