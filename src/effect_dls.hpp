#pragma once

#include "effect_simple.hpp"
#include "config.hpp"
#include "logical_device.hpp"

#include <span>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    class DlsEffect final : public SimpleEffect
    {
    public:
        DlsEffect(LogicalDevice*           pLogicalDevice,
                  VkFormat                 format,
                  VkExtent2D               imageExtent,
                  std::span<const VkImage> inputImages,
                  std::span<const VkImage> outputImages,
                  Config*                  pConfig);
        ~DlsEffect() override;
    };
} // namespace vkBasalt
