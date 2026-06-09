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
            for (auto [idx, entry] : std::views::enumerate(specMapEntrys))
            {
                entry = {.constantID = static_cast<uint32_t>(idx), .offset = static_cast<uint32_t>(sizeof(int32_t) * idx), .size = sizeof(int32_t)};
            }
            return specMapEntrys;
        }()};

        std::array specData = {height, usingPNG};

        VkSpecializationInfo fragmentSpecializationInfo{.mapEntryCount = std::size(specMapEntrys),
                                                        .pMapEntries   = std::data(specMapEntrys),
                                                        .dataSize      = std::span{specMapEntrys}.size_bytes(),
                                                        .pData         = std::data(specData)};

        pVertexSpecInfo   = nullptr;
        pFragmentSpecInfo = std::addressof(fragmentSpecializationInfo);

        const VkExtent3D lutImageExtent{
            .width = static_cast<uint32_t>(height), .height = static_cast<uint32_t>(height), .depth = static_cast<uint32_t>(height)};

        lutImage = createImage(pLogicalDevice,
                               lutImageExtent,
                               VK_FORMAT_R8G8B8A8_UNORM, // TODO search for format and save it
                               VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               lutMemory);

        uploadToImage(pLogicalDevice, lutImage, lutImageExtent, height * height * height * 4, pixels);

        if (usingPNG != 0)
        {
            stbi_image_free(pixels);
        }

        lutImageView = createImageView(pLogicalDevice, VK_FORMAT_R8G8B8A8_UNORM, lutImage, VK_IMAGE_VIEW_TYPE_3D);

        lutDescriptorSetLayout = createImageSamplerDescriptorSetLayout(pLogicalDevice, 1);
        descriptorSetLayouts.push_back(lutDescriptorSetLayout);

        VkDescriptorPoolSize imagePoolSize{.type = imagePoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                           .descriptorCount = imagePoolSize.descriptorCount = 1};

        lutDescriptorPool = createDescriptorPool(pLogicalDevice, std::span{std::addressof(imagePoolSize), 1U});

        init(pLogicalDevice, format, imageExtent, inputImages, outputImages, pConfig);

        // TODO: better solution for single image view
        lutDescriptorSet =
            allocateAndWriteImageSamplerDescriptorSets(pLogicalDevice,
                                                       lutDescriptorPool,
                                                       lutDescriptorSetLayout,
                                                       {sampler},
                                                       std::vector<std::vector<VkImageView>>(1, std::vector<VkImageView>(1, lutImageView)))
                .front();
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
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, std::addressof(lutDescriptorSet), 0, nullptr);
        SimpleEffect::applyEffect(imageIndex, commandBuffer);
    }

} // namespace vkBasalt
