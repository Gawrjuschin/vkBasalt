#include "effect_lut.hpp"
#include "logical_device.hpp"
#include "config.hpp"
#include "effect_simple.hpp"
#include "lut_cube.hpp"
#include "image_view.hpp"
#include "descriptor_set.hpp"
#include "image.hpp"
#include "shader_sources.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <ranges>
#include <vector>

#include <stb_image.h>

#include <vulkan/vulkan_core.h>

#include <logger.hpp>

namespace vkBasalt
{
    LutEffect::LutEffect(LogicalDevice*           pLogicalDevice,
                         VkFormat                 format,
                         VkExtent2D               imageExtent,
                         std::span<const VkImage> inputImages,
                         std::span<const VkImage> outputImages,
                         Config*                  pConfig)
    {
        vertexCode   = full_screen_triangle_vert;
        fragmentCode = lut_frag;

        const auto lutFile = pConfig->getOption<std::string>("lutFile");

        int        height{};
        LutCube  lutCube{};
        stbi_uc* pixels{};
        const auto usingPNG = static_cast<int32_t>(not lutFile.contains(".cube") && not lutFile.contains(".CUBE"));
        if (usingPNG == 0)
        {
            lutCube = LutCube{lutFile};
            pixels  = std::data(lutCube.colorCube);
            height  = lutCube.size;
        }
        else
        {
            int channels{};
            int width{};
            pixels = stbi_load(lutFile.c_str(), std::addressof(width), std::addressof(height), std::addressof(channels), STBI_rgb_alpha);
            if (width != height * height)
            {
                Logger::err("bad lut");
            }
        }

        constexpr static auto specMapEntrys{[] {
            constexpr static std::size_t               size{2};
            std::array<VkSpecializationMapEntry, size> specMapEntrys{}; // constexpr function
            for (auto [i, entry] : std::views::enumerate(specMapEntrys))
            {
                entry.constantID = i;
                entry.offset     = sizeof(int32_t) * i;
                entry.size       = sizeof(int32_t);
            }
            return specMapEntrys;
        }()};

        std::array specData = {height, usingPNG};

        VkSpecializationInfo fragmentSpecializationInfo;
        fragmentSpecializationInfo.mapEntryCount = std::size(specMapEntrys);
        fragmentSpecializationInfo.pMapEntries   = std::data(specMapEntrys);
        fragmentSpecializationInfo.dataSize      = std::span{specMapEntrys}.size_bytes();
        fragmentSpecializationInfo.pData         = std::data(specData);

        pVertexSpecInfo   = nullptr;
        pFragmentSpecInfo = &fragmentSpecializationInfo;

        const VkExtent3D lutImageExtent{
            .width = static_cast<uint32_t>(height), .height = static_cast<uint32_t>(height), .depth = static_cast<uint32_t>(height)};

        lutImage = createImages(pLogicalDevice,
                                1,
                                lutImageExtent,
                                VK_FORMAT_R8G8B8A8_UNORM, // TODO search for format and save it
                                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                lutMemory)[0];

        uploadToImage(pLogicalDevice, lutImage, lutImageExtent, height * height * height * 4, pixels);

        if (usingPNG != 0)
        {
            stbi_image_free(pixels);
        }

        lutImageView = createImageViews(pLogicalDevice, VK_FORMAT_R8G8B8A8_UNORM, std::span{std::addressof(lutImage), 1U}, VK_IMAGE_VIEW_TYPE_3D)[0];

        lutDescriptorSetLayout = createImageSamplerDescriptorSetLayout(pLogicalDevice, 1);
        descriptorSetLayouts.push_back(lutDescriptorSetLayout);

        VkDescriptorPoolSize imagePoolSize;
        imagePoolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        imagePoolSize.descriptorCount = 1;

        lutDescriptorPool = createDescriptorPool(pLogicalDevice, std::span{std::addressof(imagePoolSize), 1U});

        init(pLogicalDevice, format, imageExtent, inputImages, outputImages, pConfig);

        lutDescriptorSet =
            allocateAndWriteImageSamplerDescriptorSets(pLogicalDevice,
                                                       lutDescriptorPool,
                                                       lutDescriptorSetLayout,
                                                       {sampler},
                                                       std::vector<std::vector<VkImageView>>(1, std::vector<VkImageView>(1, lutImageView)))[0];
    }

    LutEffect::~LutEffect()
    {
        pLogicalDevice->vkd.DestroyImageView(pLogicalDevice->device, lutImageView, nullptr);
        pLogicalDevice->vkd.DestroyImage(pLogicalDevice->device, lutImage, nullptr);
        pLogicalDevice->vkd.DestroyDescriptorSetLayout(pLogicalDevice->device, lutDescriptorSetLayout, nullptr);
        pLogicalDevice->vkd.DestroyDescriptorPool(pLogicalDevice->device, lutDescriptorPool, nullptr);
        pLogicalDevice->vkd.FreeMemory(pLogicalDevice->device, lutMemory, nullptr);
    }

    void LutEffect::applyEffect(uint32_t imageIndex, VkCommandBuffer commandBuffer)
    {
        pLogicalDevice->vkd.CmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &(lutDescriptorSet), 0, nullptr);
        SimpleEffect::applyEffect(imageIndex, commandBuffer);
    }

} // namespace vkBasalt
