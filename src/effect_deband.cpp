#include "effect_deband.hpp"
#include "logical_device.hpp"
#include "config.hpp"
#include "shader_sources.hpp"

#include <array>
#include <memory>
#include <span>
#include <cstdint>
#include <vector>
#include <iterator>

#include <vulkan/vulkan_core.h>

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

        struct
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
        } debandOptions{};

        debandOptions.screenWidth         = static_cast<float>(imageExtent.width);
        debandOptions.screenHeight        = static_cast<float>(imageExtent.height);
        debandOptions.reverseScreenWidth  = 1.0F / imageExtent.width;
        debandOptions.reverseScreenHeight = 1.0F / imageExtent.height;

        // get Options
        debandOptions.debandAvgdiff = pConfig->getOption<float>("debandAvgdiff", 3.4F);
        debandOptions.debandMaxdiff = pConfig->getOption<float>("debandMaxdiff", 6.8F);
        debandOptions.debandMiddiff = pConfig->getOption<float>("debandMiddiff", 3.3F);
        debandOptions.range         = pConfig->getOption<float>("debandRange", 16.0F);
        debandOptions.iterations    = pConfig->getOption<int32_t>("debandIterations", 4);

        std::array<VkSpecializationMapEntry, 9U> specMapEntrys{}; // TODO: why 9???
        for (uint32_t i = 0; i < std::size(specMapEntrys); ++i)
        {
            specMapEntrys[i].constantID = i;
            specMapEntrys[i].offset     = sizeof(float) * i; // TODO not clean to assume that sizeof(int32_t) == sizeof(float)
            specMapEntrys[i].size       = sizeof(float);
        }

        VkSpecializationInfo specializationInfo;
        specializationInfo.mapEntryCount = std::size(specMapEntrys);
        specializationInfo.pMapEntries   = std::data(specMapEntrys);
        specializationInfo.dataSize      = sizeof(decltype(debandOptions));
        specializationInfo.pData         = std::addressof(debandOptions);

        pVertexSpecInfo   = nullptr;
        pFragmentSpecInfo = std::addressof(specializationInfo);

        init(pLogicalDevice, format, imageExtent, inputImages, outputImages, pConfig);
    }

    DebandEffect::~DebandEffect() = default;

} // namespace vkBasalt
