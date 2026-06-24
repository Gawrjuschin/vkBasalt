#include "effect_dls.hpp"
#include "logical_device.hpp"
#include "config.hpp"
#include "shader_sources.hpp"

#include <array>
#include <memory>
#include <span>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    DlsEffect::DlsEffect(LogicalDevice*           pLogicalDevice,
                         VkFormat                 format,
                         VkExtent2D               imageExtent,
                         std::span<const VkImage> inputImages,
                         std::span<const VkImage> outputImages,
                         const Config&            config)
    {
        const auto sharpness = config.getOption<float>("dlsSharpness", 0.5F);
        const auto denoise   = config.getOption<float>("dlsDenoise", 0.17F);

        const std::array specData{sharpness, denoise};

        vertexCode   = full_screen_triangle_vert;
        fragmentCode = dls_frag;

        constexpr static std::array mapEntries{VkSpecializationMapEntry{.constantID = 0, .offset = 0, .size = sizeof(float)},
                                               VkSpecializationMapEntry{.constantID = 1, .offset = sizeof(float), .size = sizeof(float)}};

        const VkSpecializationInfo fragmentSpecializationInfo{.mapEntryCount = 1, // TODO: why 1 and not std::size(mapEntries)?
                                                              .pMapEntries   = std::data(mapEntries),
                                                              .dataSize      = std::span{specData}.size_bytes(),
                                                              .pData         = std::data(specData)};

        init(pLogicalDevice, format, imageExtent, inputImages, outputImages, nullptr, std::addressof(fragmentSpecializationInfo));
    }

    DlsEffect::~DlsEffect() = default;

} // namespace vkBasalt
