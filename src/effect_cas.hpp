#ifndef EFFECT_CAS_HPP_INCLUDED
#define EFFECT_CAS_HPP_INCLUDED

#include "effect_simple.hpp"
#include "config.hpp"
#include "logical_device.hpp"

#include <span>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    class CasEffect final : public SimpleEffect
    {
    public:
        CasEffect(LogicalDevice*           pLogicalDevice,
                  VkFormat                 format,
                  VkExtent2D               imageExtent,
                  std::span<const VkImage> inputImages,
                  std::span<const VkImage> outputImages,
                  Config*                  pConfig);

        CasEffect(const CasEffect&)            = delete;
        CasEffect& operator=(const CasEffect&) = delete;
        CasEffect(CasEffect&&)                 = delete;
        CasEffect& operator=(CasEffect&&)      = delete;

        ~CasEffect() override;
    };
} // namespace vkBasalt

#endif // EFFECT_CAS_HPP_INCLUDED
