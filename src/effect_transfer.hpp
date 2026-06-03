#ifndef EFFECT_TRANSFER_HPP_INCLUDED
#define EFFECT_TRANSFER_HPP_INCLUDED

#include "effect.hpp"
#include "config.hpp"
#include "logical_device.hpp"

#include <cstdint>
#include <vector>
#include <span>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    class TransferEffect final : public Effect
    {
    public:
        TransferEffect(LogicalDevice*           pLogicalDevice,
                       VkFormat                 format,
                       VkExtent2D               imageExtent,
                       std::span<const VkImage> inputImages,
                       std::span<const VkImage> outputImages,
                       Config*                  pConfig);
        void applyEffect(uint32_t imageIndex, VkCommandBuffer commandBuffer) override;
        ~TransferEffect() override;

    private:
        LogicalDevice*       pLogicalDevice;
        std::vector<VkImage> inputImages;
        std::vector<VkImage> outputImages;
        VkExtent2D           imageExtent;
        VkFormat             format;
        Config*              pConfig;
    };
} // namespace vkBasalt
#endif // EFFECT_TRANSFER_HPP_INCLUDED
