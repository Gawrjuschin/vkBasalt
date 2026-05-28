#pragma once

#include "effect_simple.hpp"
#include "config.hpp"

#include <vector>

namespace vkBasalt
{
    class DlsEffect : public SimpleEffect
    {
    public:
        DlsEffect(LogicalDevice*       pLogicalDevice,
                  VkFormat             format,
                  VkExtent2D           imageExtent,
                  std::vector<VkImage> inputImages,
                  std::vector<VkImage> outputImages,
                  Config*              pConfig);
        ~DlsEffect();
    };
} // namespace vkBasalt
