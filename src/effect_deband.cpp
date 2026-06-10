#include "effect_deband.hpp"
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

namespace
{
    struct DebandOptions
    {
        float   screenWidth;
        float   screenHeight;
        float   reverseScreenWidth;
        float   reverseScreenHeight;
        float   debandAvgdiff;
        float   debandMaxdiff;
        float   debandMiddiff;
        float   range;
        int32_t iterations;
    };

} // namespace

namespace vkBasalt
{
    DebandEffect::DebandEffect(LogicalDevice*           pLogicalDevice,
                               VkFormat                 format,
                               VkExtent2D               imageExtent,
                               std::span<const VkImage> inputImages,
                               std::span<const VkImage> outputImages,
                               Config*                  pConfig)
    {
        vertexCode   = full_screen_triangle_vert;
        fragmentCode = deband_frag;

        const DebandOptions debandOptions{
            .screenWidth         = static_cast<float>(imageExtent.width),
            .screenHeight        = static_cast<float>(imageExtent.height),
            .reverseScreenWidth  = 1.0F / static_cast<float>(imageExtent.width),
            .reverseScreenHeight = 1.0F / static_cast<float>(imageExtent.height),
            .debandAvgdiff       = pConfig->getOption<float>("debandAvgdiff", 3.4F),
            .debandMaxdiff       = pConfig->getOption<float>("debandMaxdiff", 6.8F),
            .debandMiddiff       = pConfig->getOption<float>("debandMiddiff", 3.3F),
            .range               = pConfig->getOption<float>("debandRange", 16.0F),
            .iterations          = pConfig->getOption<int32_t>("debandIterations", 4),
        };

        static_assert(sizeof(int32_t) == sizeof(float));

        constexpr static auto specMapEntrys{[] {
            static constexpr std::size_t               size{9}; // TODO: why 9???
            std::array<VkSpecializationMapEntry, size> specMapEntrys{};
            for (auto [idx, specMapEntry] : specMapEntrys | std::views::enumerate)
            {
                specMapEntry = {
                    .constantID = static_cast<uint32_t>(idx), .offset = static_cast<uint32_t>(sizeof(float) * idx), .size = sizeof(float)};
            }
            return specMapEntrys;
        }()};

        const VkSpecializationInfo specializationInfo{
            .mapEntryCount = std::size(specMapEntrys),
            .pMapEntries   = std::data(specMapEntrys),
            .dataSize      = sizeof(DebandOptions),
            .pData         = std::addressof(debandOptions),
        };

        pVertexSpecInfo   = nullptr;
        pFragmentSpecInfo = std::addressof(specializationInfo);

        init(pLogicalDevice, format, imageExtent, inputImages, outputImages, pConfig);
    }

    DebandEffect::~DebandEffect() = default;

} // namespace vkBasalt
