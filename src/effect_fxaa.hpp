#ifndef EFFECT_FXAA_HPP_INCLUDED
#define EFFECT_FXAA_HPP_INCLUDED

#include "effect_simple.hpp"
#include "config.hpp"

#include <vector>

namespace vkBasalt
{
    class FxaaEffect : public SimpleEffect
    {
    public:
        FxaaEffect(LogicalDevice*       pLogicalDevice,
                   VkFormat             format,
                   VkExtent2D           imageExtent,
                   std::vector<VkImage> inputImages,
                   std::vector<VkImage> outputImages,
                   Config*              pConfig);
        ~FxaaEffect();
    };
} // namespace vkBasalt

#endif // EFFECT_FXAA_HPP_INCLUDED
