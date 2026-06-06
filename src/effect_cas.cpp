#include "effect_cas.hpp"
#include "logical_device.hpp"
#include "config.hpp"
#include "shader_sources.hpp"
#include <memory>
#include <vulkan/vulkan_core.h>
#include <span>

namespace vkBasalt
{
    CasEffect::CasEffect(LogicalDevice*           pLogicalDevice,
                         VkFormat                 format,
                         VkExtent2D               imageExtent,
                         std::span<const VkImage> inputImages,
                         std::span<const VkImage> outputImages,
                         Config*                  pConfig)
    {

        const auto sharpness = pConfig->getOption<float>("casSharpness", 0.4F);

        vertexCode   = full_screen_triangle_vert;
        fragmentCode = cas_frag;

        constexpr static VkSpecializationMapEntry sharpnessMapEntry{
            .constantID = 0,
            .offset     = 0,
            .size       = sizeof(float),
        };

        VkSpecializationInfo fragmentSpecializationInfo{
            .mapEntryCount = 1, .pMapEntries = std::addressof(sharpnessMapEntry), .dataSize = sizeof(float), .pData = std::addressof(sharpness)};

        pVertexSpecInfo   = nullptr;
        pFragmentSpecInfo = std::addressof(fragmentSpecializationInfo);

        init(pLogicalDevice, format, imageExtent, inputImages, outputImages, pConfig);
    }

    CasEffect::~CasEffect() = default;

} // namespace vkBasalt
