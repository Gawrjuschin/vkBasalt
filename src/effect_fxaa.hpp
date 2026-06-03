#ifndef EFFECT_FXAA_HPP_INCLUDED
#define EFFECT_FXAA_HPP_INCLUDED

#include "effect_simple.hpp"
#include "config.hpp"
#include "logical_device.hpp"

#include <span>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    class FxaaEffect final : public SimpleEffect
    {
    public:
        FxaaEffect(LogicalDevice*           pLogicalDevice,
                   VkFormat                 format,
                   VkExtent2D               imageExtent,
                   std::span<const VkImage> inputImages,
                   std::span<const VkImage> outputImages,
                   Config*                  pConfig);
        ~FxaaEffect() override;
    };
} // namespace vkBasalt

#endif // EFFECT_FXAA_HPP_INCLUDED
