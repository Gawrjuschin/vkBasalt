#include "sampler.hpp"
#include "logical_device.hpp"
#include "effect_module.hpp"
#include "vulkan_include.hpp"
#include <memory>
#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    VkSampler createSampler(LogicalDevice* pLogicalDevice)
    {
        VkSampler sampler{};

        VkSamplerCreateInfo samplerCreateInfo{.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                                              .pNext                   = nullptr,
                                              .flags                   = 0,
                                              .magFilter               = VK_FILTER_LINEAR,
                                              .minFilter               = VK_FILTER_LINEAR,
                                              .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                                              .addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                              .addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                              .addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                              .mipLodBias              = 0.0F,
                                              .anisotropyEnable        = VK_FALSE,
                                              .maxAnisotropy           = 16,
                                              .compareEnable           = VK_FALSE,
                                              .compareOp               = VK_COMPARE_OP_ALWAYS,
                                              .minLod                  = 0.0F,
                                              .maxLod                  = 0.0F,
                                              .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                                              .unnormalizedCoordinates = VK_FALSE};

        const auto result =
            pLogicalDevice->vkd.CreateSampler(pLogicalDevice->device, std::addressof(samplerCreateInfo), nullptr, std::addressof(sampler));
        AssertVulkan(result);
        return sampler;
    }

    VkSampler createReshadeSampler(LogicalDevice* pLogicalDevice, const reshadefx::sampler_info& samplerInfo)
    {
        VkSampler sampler{};

        VkFilter            minFilter{};
        VkFilter            magFilter{};
        VkSamplerMipmapMode mipmapMode{};
        convertReshadeFilter(samplerInfo.filter, minFilter, magFilter, mipmapMode);

        VkSamplerCreateInfo samplerCreateInfo{.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                                              .pNext                   = nullptr,
                                              .flags                   = 0,
                                              .magFilter               = magFilter,
                                              .minFilter               = minFilter,
                                              .mipmapMode              = mipmapMode,
                                              .addressModeU            = convertReshadeAddressMode(samplerInfo.address_u),
                                              .addressModeV            = convertReshadeAddressMode(samplerInfo.address_v),
                                              .addressModeW            = convertReshadeAddressMode(samplerInfo.address_w),
                                              .mipLodBias              = samplerInfo.lod_bias,
                                              .anisotropyEnable        = VK_FALSE,
                                              .maxAnisotropy           = 16,
                                              .compareEnable           = VK_FALSE,
                                              .compareOp               = VK_COMPARE_OP_ALWAYS,
                                              .minLod                  = samplerInfo.min_lod,
                                              .maxLod                  = samplerInfo.max_lod,
                                              .borderColor             = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
                                              .unnormalizedCoordinates = VK_FALSE};

        const auto result =
            pLogicalDevice->vkd.CreateSampler(pLogicalDevice->device, std::addressof(samplerCreateInfo), nullptr, std::addressof(sampler));
        AssertVulkan(result);
        return sampler;
    }

    VkSamplerAddressMode convertReshadeAddressMode(const reshadefx::texture_address_mode& addressMode)
    {
        switch (addressMode)
        {
            case reshadefx::texture_address_mode::wrap: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case reshadefx::texture_address_mode::mirror: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            case reshadefx::texture_address_mode::clamp: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case reshadefx::texture_address_mode::border: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        }
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }

    void
    convertReshadeFilter(const reshadefx::texture_filter& textureFilter, VkFilter& minFilter, VkFilter& magFilter, VkSamplerMipmapMode& mipmapMode)
    {
        switch (textureFilter)
        {
            case reshadefx::texture_filter::min_mag_mip_point:
                minFilter  = VK_FILTER_NEAREST;
                magFilter  = VK_FILTER_NEAREST;
                mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
                return;
            case reshadefx::texture_filter::min_mag_point_mip_linear:
                minFilter  = VK_FILTER_NEAREST;
                magFilter  = VK_FILTER_NEAREST;
                mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                return;
            case reshadefx::texture_filter::min_point_mag_linear_mip_point:
                minFilter  = VK_FILTER_NEAREST;
                magFilter  = VK_FILTER_LINEAR;
                mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
                return;
            case reshadefx::texture_filter::min_point_mag_mip_linear:
                minFilter  = VK_FILTER_NEAREST;
                magFilter  = VK_FILTER_LINEAR;
                mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                return;
            case reshadefx::texture_filter::min_linear_mag_mip_point:
                minFilter  = VK_FILTER_LINEAR;
                magFilter  = VK_FILTER_NEAREST;
                mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
                return;
            case reshadefx::texture_filter::min_linear_mag_point_mip_linear:
                minFilter  = VK_FILTER_LINEAR;
                magFilter  = VK_FILTER_NEAREST;
                mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                return;
            case reshadefx::texture_filter::min_mag_linear_mip_point:
                minFilter  = VK_FILTER_LINEAR;
                magFilter  = VK_FILTER_LINEAR;
                mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
                return;
            case reshadefx::texture_filter::min_mag_mip_linear:
                minFilter  = VK_FILTER_LINEAR;
                magFilter  = VK_FILTER_LINEAR;
                mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                return;
        }
    }
} // namespace vkBasalt
