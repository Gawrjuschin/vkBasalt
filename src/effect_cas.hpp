#ifndef EFFECT_CAS_HPP_INCLUDED
#define EFFECT_CAS_HPP_INCLUDED

#include "effect_simple.hpp"
#include "config.hpp"

#include <vector>

namespace vkBasalt
{
    class CasEffect final : public SimpleEffect
    {
    public:
        CasEffect(LogicalDevice*       pLogicalDevice,
                  VkFormat             format,
                  VkExtent2D           imageExtent,
                  std::vector<VkImage> inputImages,
                  std::vector<VkImage> outputImages,
                  Config*              pConfig);
        ~CasEffect() override;
    };
} // namespace vkBasalt

#endif // EFFECT_CAS_HPP_INCLUDED
