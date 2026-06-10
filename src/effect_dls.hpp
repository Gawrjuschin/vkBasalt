#ifndef VKBASALT_EFFECT_DLS_HPP
#define VKBASALT_EFFECT_DLS_HPP

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

        DlsEffect(const DlsEffect&)            = delete;
        DlsEffect& operator=(const DlsEffect&) = delete;
        DlsEffect(DlsEffect&&)                 = delete;
        DlsEffect& operator=(DlsEffect&&)      = delete;

        ~DlsEffect() override;
    };
} // namespace vkBasalt

#endif // VKBASALT_EFFECT_DLS_HPP
