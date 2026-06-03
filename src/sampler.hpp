#ifndef SAMPLER_HPP_INCLUDED
#define SAMPLER_HPP_INCLUDED

#include "logical_device.hpp"

#include <effect_module.hpp>
#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    VkSampler createSampler(LogicalDevice* pLogicalDevice);

    VkSampler createReshadeSampler(LogicalDevice* pLogicalDevice, const reshadefx::sampler_info& samplerInfo);

    VkSamplerAddressMode convertReshadeAddressMode(const reshadefx::texture_address_mode& addressMode);

    void
    convertReshadeFilter(const reshadefx::texture_filter& textureFilter, VkFilter& minFilter, VkFilter& magFilter, VkSamplerMipmapMode& mipmapMode);
} // namespace vkBasalt

#endif // SAMPLER_HPP_INCLUDED
