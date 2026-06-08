#ifndef IMAGE_VIEW_HPP_INCLUDED
#define IMAGE_VIEW_HPP_INCLUDED

#include "logical_device.hpp"

#include <cstdint>
#include <span>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    VkImageView createImageView(LogicalDevice*     pLogicalDevice,
                                VkFormat           format,
                                const VkImage&     image,
                                VkImageViewType    viewType   = VK_IMAGE_VIEW_TYPE_2D,
                                VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                uint32_t           mipLevels  = 1);

    std::vector<VkImageView> createImageViews(LogicalDevice*           pLogicalDevice,
                                              VkFormat                 format,
                                              std::span<const VkImage> images,
                                              VkImageViewType          viewType   = VK_IMAGE_VIEW_TYPE_2D,
                                              VkImageAspectFlags       aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                              uint32_t                 mipLevels  = 1);
} // namespace vkBasalt

#endif // IMAGE_VIEW_HPP_INCLUDED
