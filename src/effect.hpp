#ifndef EFFECT_HPP_INCLUDED
#define EFFECT_HPP_INCLUDED

#include <cstdint>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    class Effect
    {
    public:
        void virtual applyEffect(uint32_t imageIndex, VkCommandBuffer commandBuffer) = 0;
        void virtual updateEffect(){};
        void virtual useDepthImage(VkImageView /*depthImageView*/){};

        virtual ~Effect() = default;

    private:
    };
} // namespace vkBasalt

#endif // EFFECT_HPP_INCLUDED
