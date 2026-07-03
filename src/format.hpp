#ifndef FORMAT_HPP_INCLUDED
#define FORMAT_HPP_INCLUDED

#include "logical_device.hpp"

#include <span>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    // Returns a matching sRGB format to a UNORM format if it exist, else returns format
    VkFormat convertToSRGB(VkFormat format) noexcept;
    // Returns a matching UNORM format to a sRGB format if it exist, else returns format
    VkFormat convertToUNORM(VkFormat format) noexcept;
    // Returns true if format is SRGB
    inline bool isSRGB(VkFormat format) noexcept
    {
        return convertToUNORM(format) != format;
    }
    // Returns true if format is UNORM
    // TODO currently return false if format is UNORM and no matching sRGB format exist
    inline bool isUNORM(VkFormat format) noexcept
    {
        return convertToSRGB(format) != format;
    }

    VkFormat getSupportedFormat(LogicalDevice*            pLogicalDevice,
                                std::span<const VkFormat> formats,
                                VkFormatFeatureFlags      features,
                                VkImageTiling             tiling = VK_IMAGE_TILING_OPTIMAL);

    VkFormat getStencilFormat(LogicalDevice* pLogicalDevice) noexcept;

    bool isDepthFormat(VkFormat format) noexcept;

    bool isStencilFormat(VkFormat format) noexcept;
} // namespace vkBasalt

#endif // FORMAT_HPP_INCLUDED
