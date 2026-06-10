#include "effect_fxaa.hpp"
#include "logical_device.hpp"
#include "config.hpp"
#include "shader_sources.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <ranges>
#include <span>
#include <vector>

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

        constexpr auto static specMapEntrys{[] {
            constexpr static std::size_t               size{5};
            std::array<VkSpecializationMapEntry, size> specMapEntrys{}; // TODO: why 5

            for (auto [idx, entry] : std::views::enumerate(specMapEntrys))
            {
                entry = {.constantID = static_cast<uint32_t>(idx), .offset = static_cast<uint32_t>(sizeof(float) * idx), .size = sizeof(float)};
            }
            return specMapEntrys;
        }()};

        const std::array specData = {fxaaQualitySubpix,
                                     fxaaQualityEdgeThreshold,
                                     fxaaQualityEdgeThresholdMin,
                                     static_cast<float>(imageExtent.width),
                                     static_cast<float>(imageExtent.height)};

        const VkSpecializationInfo fragmentSpecializationInfo{.mapEntryCount = std::size(specMapEntrys),
                                                              .pMapEntries   = std::data(specMapEntrys),
                                                              .dataSize      = std::span{specData}.size_bytes(),
                                                              .pData         = std::data(specData)};

        pVertexSpecInfo   = nullptr;
        pFragmentSpecInfo = std::addressof(fragmentSpecializationInfo);

        init(pLogicalDevice, format, imageExtent, inputImages, outputImages, pConfig);
    }

    FxaaEffect::~FxaaEffect() = default;

} // namespace vkBasalt
