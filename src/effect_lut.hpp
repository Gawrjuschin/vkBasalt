#ifndef EFFECT_LUT_HPP_INCLUDED
#define EFFECT_LUT_HPP_INCLUDED

#include "effect_simple.hpp"
#include "config.hpp"
#include "logical_device.hpp"

#include <cstdint>
#include <span>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    class LutEffect final : public SimpleEffect
    {
    public:
        LutEffect(LogicalDevice*           pLogicalDevice,
                  VkFormat                 format,
                  VkExtent2D               imageExtent,
                  std::span<const VkImage> inputImages,
                  std::span<const VkImage> outputImages,
                  Config*                  pConfig);
        ~LutEffect() override;
        void applyEffect(uint32_t imageIndex, VkCommandBuffer commandBuffer) override;

    private:
        VkImage               lutImage;
        VkDeviceMemory        lutMemory;
        VkImageView           lutImageView;
        VkDescriptorSetLayout lutDescriptorSetLayout;
        VkDescriptorPool      lutDescriptorPool;
        VkDescriptorSet       lutDescriptorSet;
    };
} // namespace vkBasalt

#endif // EFFECT_LUT_HPP_INCLUDED
