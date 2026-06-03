#include "effect_fxaa.hpp"
#include "logical_device.hpp"
#include "config.hpp"
#include "shader_sources.hpp"

#include <array>
#include <memory>
#include <span>
#include <vector>
#include <cstdint>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    FxaaEffect::FxaaEffect(LogicalDevice*           pLogicalDevice,
                           VkFormat                 format,
                           VkExtent2D               imageExtent,
                           std::span<const VkImage> inputImages,
                           std::span<const VkImage> outputImages,
                           Config*                  pConfig)
    {
        const auto fxaaQualitySubpix           = pConfig->getOption<float>("fxaaQualitySubpix", 0.75F);
        const auto fxaaQualityEdgeThreshold    = pConfig->getOption<float>("fxaaQualityEdgeThreshold", 0.125F);
        const auto fxaaQualityEdgeThresholdMin = pConfig->getOption<float>("fxaaQualityEdgeThresholdMin", 0.0312F);

        vertexCode   = full_screen_triangle_vert;
        fragmentCode = fxaa_frag;

        std::array<VkSpecializationMapEntry, 5U> specMapEntrys{}; // TODO: why 5

        for (uint32_t i = 0; i < std::size(specMapEntrys); ++i)
        {
            specMapEntrys[i].constantID = i;
            specMapEntrys[i].offset     = sizeof(float) * i;
            specMapEntrys[i].size       = sizeof(float);
        }
        std::array specData = {fxaaQualitySubpix,
                               fxaaQualityEdgeThreshold,
                               fxaaQualityEdgeThresholdMin,
                               static_cast<float>(imageExtent.width),
                               static_cast<float>(imageExtent.height)};

        VkSpecializationInfo fragmentSpecializationInfo;
        fragmentSpecializationInfo.mapEntryCount = std::size(specMapEntrys);
        fragmentSpecializationInfo.pMapEntries   = std::data(specMapEntrys);
        fragmentSpecializationInfo.dataSize      = std::span{specData}.size_bytes();
        fragmentSpecializationInfo.pData         = std::data(specData);

        pVertexSpecInfo   = nullptr;
        pFragmentSpecInfo = std::addressof(fragmentSpecializationInfo);

        init(pLogicalDevice, format, imageExtent, inputImages, outputImages, pConfig);
    }

    FxaaEffect::~FxaaEffect() = default;

} // namespace vkBasalt
