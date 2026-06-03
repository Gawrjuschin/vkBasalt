#include "effect_dls.hpp"
#include "logical_device.hpp"
#include "config.hpp"
#include "shader_sources.hpp"

#include <array>
#include <span>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    DlsEffect::DlsEffect(LogicalDevice*           pLogicalDevice,
                         VkFormat                 format,
                         VkExtent2D               imageExtent,
                         std::span<const VkImage> inputImages,
                         std::span<const VkImage> outputImages,
                         Config*                  pConfig)
    {
        const auto sharpness = pConfig->getOption<float>("dlsSharpness", 0.5F);
        const auto denoise   = pConfig->getOption<float>("dlsDenoise", 0.17F);

        std::array specData{sharpness, denoise};

        vertexCode   = full_screen_triangle_vert;
        fragmentCode = dls_frag;

        VkSpecializationMapEntry mapEntries[2];
        mapEntries[0].constantID = 0;
        mapEntries[0].offset     = 0;
        mapEntries[0].size       = sizeof(float);
        mapEntries[1].constantID = 1;
        mapEntries[1].offset     = sizeof(float);
        mapEntries[1].size       = sizeof(float);

        VkSpecializationInfo fragmentSpecializationInfo;
        fragmentSpecializationInfo.mapEntryCount = 1;
        fragmentSpecializationInfo.pMapEntries   = mapEntries;
        fragmentSpecializationInfo.dataSize      = std::span{specData}.size_bytes();
        fragmentSpecializationInfo.pData         = std::data(specData);

        pVertexSpecInfo   = nullptr;
        pFragmentSpecInfo = &fragmentSpecializationInfo;

        init(pLogicalDevice, format, imageExtent, inputImages, outputImages, pConfig);
    }

    DlsEffect::~DlsEffect() = default;

} // namespace vkBasalt
