#ifndef EFFECT_DEBAND_HPP_INCLUDED
#define EFFECT_DEBAND_HPP_INCLUDED

#include "effect_simple.hpp"
#include "config.hpp"
#include "logical_device.hpp"

#include <span>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    class DebandEffect final : public SimpleEffect
    {
    public:
        DebandEffect(LogicalDevice*           pLogicalDevice,
                     VkFormat                 format,
                     VkExtent2D               imageExtent,
                     std::span<const VkImage> inputImages,
                     std::span<const VkImage> outputImages,
                     const Config&            config);

        DebandEffect(const DebandEffect&)            = delete;
        DebandEffect& operator=(const DebandEffect&) = delete;
        DebandEffect(DebandEffect&&)                 = delete;
        DebandEffect& operator=(DebandEffect&&)      = delete;

        ~DebandEffect() override;
    };
} // namespace vkBasalt

#endif // EFFECT_DEBAND_HPP_INCLUDED
