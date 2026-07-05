#include "effect_reshade.hpp"
#include "config.hpp"
#include "effect_module.hpp"
#include "effect_preprocessor.hpp"
#include "effect_parser.hpp"
#include "effect_codegen.hpp"
#include "image_view.hpp"
#include "descriptor_set.hpp"
#include "buffer.hpp"
#include "graphics_pipeline.hpp"
#include "framebuffer.hpp"
#include "logical_device.hpp"
#include "reshade_uniforms.hpp"
#include "sampler.hpp"
#include "image.hpp"
#include "format.hpp"
#include "util.hpp"
#include "vulkan_include.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <climits>
#include <cstdlib>
#include <cassert>
#include <iterator>
#include <memory>
#include <ranges>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <stb_image.h>
#include <stb_image_dds.h>
#include <stb_image_resize.h>

#include <vulkan/vulkan_core.h>

#include <logger.hpp>

namespace vkBasalt
{
    namespace
    {
        VkFormat convertReshadeFormat(reshadefx::texture_format texFormat) noexcept
        {
            switch (texFormat)
            {
                case reshadefx::texture_format::r8: return VK_FORMAT_R8_UNORM;
                case reshadefx::texture_format::r16f: return VK_FORMAT_R16_SFLOAT;
                case reshadefx::texture_format::r32f: return VK_FORMAT_R32_SFLOAT;
                case reshadefx::texture_format::rg8: return VK_FORMAT_R8G8_UNORM;
                case reshadefx::texture_format::rg16: return VK_FORMAT_R16G16_UNORM;
                case reshadefx::texture_format::rg16f: return VK_FORMAT_R16G16_SFLOAT;
                case reshadefx::texture_format::rg32f: return VK_FORMAT_R32G32_SFLOAT;
                case reshadefx::texture_format::rgba8: return VK_FORMAT_R8G8B8A8_UNORM;
                case reshadefx::texture_format::rgba16: return VK_FORMAT_R16G16B16A16_UNORM;
                case reshadefx::texture_format::rgba16f: return VK_FORMAT_R16G16B16A16_SFLOAT;
                case reshadefx::texture_format::rgba32f: return VK_FORMAT_R32G32B32A32_SFLOAT;
                case reshadefx::texture_format::rgb10a2: return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
                default: return VK_FORMAT_UNDEFINED;
            }
        }

        VkCompareOp convertReshadeCompareOp(reshadefx::pass_stencil_func compareOp) noexcept
        {
            switch (compareOp)
            {
                case reshadefx::pass_stencil_func::never: return VK_COMPARE_OP_NEVER;
                case reshadefx::pass_stencil_func::less: return VK_COMPARE_OP_LESS;
                case reshadefx::pass_stencil_func::equal: return VK_COMPARE_OP_EQUAL;
                case reshadefx::pass_stencil_func::less_equal: return VK_COMPARE_OP_LESS_OR_EQUAL;
                case reshadefx::pass_stencil_func::greater: return VK_COMPARE_OP_GREATER;
                case reshadefx::pass_stencil_func::not_equal: return VK_COMPARE_OP_NOT_EQUAL;
                case reshadefx::pass_stencil_func::greater_equal: return VK_COMPARE_OP_GREATER_OR_EQUAL;
                case reshadefx::pass_stencil_func::always: return VK_COMPARE_OP_ALWAYS;
                default: return VK_COMPARE_OP_ALWAYS;
            }
        }

        VkStencilOp convertReshadeStencilOp(reshadefx::pass_stencil_op stencilOp) noexcept
        {
            switch (stencilOp)
            {
                case reshadefx::pass_stencil_op::zero: return VK_STENCIL_OP_ZERO;
                case reshadefx::pass_stencil_op::keep: return VK_STENCIL_OP_KEEP;
                case reshadefx::pass_stencil_op::replace: return VK_STENCIL_OP_REPLACE;
                case reshadefx::pass_stencil_op::incr_sat: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
                case reshadefx::pass_stencil_op::decr_sat: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
                case reshadefx::pass_stencil_op::invert: return VK_STENCIL_OP_INVERT;
                case reshadefx::pass_stencil_op::incr: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
                case reshadefx::pass_stencil_op::decr: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
                default: return VK_STENCIL_OP_KEEP;
            }
        }

        VkBlendOp convertReshadeBlendOp(reshadefx::pass_blend_op blendOp) noexcept
        {
            switch (blendOp)
            {
                case reshadefx::pass_blend_op::add: return VK_BLEND_OP_ADD;
                case reshadefx::pass_blend_op::subtract: return VK_BLEND_OP_SUBTRACT;
                case reshadefx::pass_blend_op::rev_subtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
                case reshadefx::pass_blend_op::min: return VK_BLEND_OP_MIN;
                case reshadefx::pass_blend_op::max: return VK_BLEND_OP_MAX;
                default: return VK_BLEND_OP_ADD;
            }
        }

        VkBlendFactor convertReshadeBlendFactor(reshadefx::pass_blend_func blendFactor) noexcept
        {
            switch (blendFactor)
            {
                case reshadefx::pass_blend_func::zero: return VK_BLEND_FACTOR_ZERO;
                case reshadefx::pass_blend_func::one: return VK_BLEND_FACTOR_ONE;
                case reshadefx::pass_blend_func::src_color: return VK_BLEND_FACTOR_SRC_COLOR;
                case reshadefx::pass_blend_func::src_alpha: return VK_BLEND_FACTOR_SRC_ALPHA;
                case reshadefx::pass_blend_func::inv_src_color: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
                case reshadefx::pass_blend_func::inv_src_alpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                case reshadefx::pass_blend_func::dst_alpha: return VK_BLEND_FACTOR_DST_ALPHA;
                case reshadefx::pass_blend_func::inv_dst_alpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
                case reshadefx::pass_blend_func::dst_color: return VK_BLEND_FACTOR_DST_COLOR;
                case reshadefx::pass_blend_func::inv_dst_color: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
                default: return VK_BLEND_FACTOR_ZERO;
            }
        }
    } // namespace

    ReshadeEffect::ReshadeEffect(LogicalDevice*           pLogicalDevice,
                                 VkFormat                 format,
                                 VkExtent2D               imageExtent,
                                 std::span<const VkImage> inputImages,
                                 std::span<const VkImage> outputImages,
                                 std::string              effectName,
                                 const Config&            config) :
        pLogicalDevice{pLogicalDevice}, inputImages(std::cbegin(inputImages), std::cend(inputImages)),
        outputImages(std::cbegin(outputImages), std::cend(outputImages)), imageExtent{imageExtent}, effectName{std::move(effectName)}, module{},
        inputOutputFormatUNORM(convertToUNORM(format)), inputOutputFormatSRGB{convertToSRGB(format)},
        inputImageViewsSRGB{createImageViews(pLogicalDevice, inputOutputFormatSRGB, inputImages)},
        inputImageViewsUNORM{createImageViews(pLogicalDevice, inputOutputFormatUNORM, inputImages)},
        outputImageViewsSRGB{createImageViews(pLogicalDevice, inputOutputFormatSRGB, outputImages)},
        outputImageViewsUNORM{createImageViews(pLogicalDevice, inputOutputFormatUNORM, outputImages)}
    {
        createReshadeModule(config);

        enumerateReshadeUniforms(module);

        uniforms = createReshadeUniforms(module);

        bufferSize = module.total_uniform_size;
        if (bufferSize != 0)
        {
            createBuffer(pLogicalDevice,
                         bufferSize,
                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         stagingBuffer,
                         stagingBufferMemory);
        }

        stencilFormat = getStencilFormat(pLogicalDevice);
        Logger::debug("Stencil Format: " + std::to_string(stencilFormat));
        textureMemory.emplace_back(VK_NULL_HANDLE);
        stencilImage = createImage(pLogicalDevice,
                                   {.width = imageExtent.width, .height = imageExtent.height, .depth = 1},
                                   stencilFormat,
                                   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                   textureMemory.back());

        stencilImageView = createImageView(
            pLogicalDevice, stencilFormat, stencilImage, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

        std::vector<std::vector<VkImageView>> imageViewVector;

        for (auto [idx, texture] : module.textures | std::views::enumerate)
        {
            textureMipLevels[texture.unique_name] = texture.levels;
            textureExtents[texture.unique_name]   = {.width = texture.width, .height = texture.height, .depth = 1};
            if (texture.semantic == "COLOR" || texture.semantic == "DEPTH")
            {
                textureImageViewsUNORM[texture.unique_name] = inputImageViewsUNORM;
                renderImageViewsUNORM[texture.unique_name]  = inputImageViewsUNORM;

                textureImageViewsSRGB[texture.unique_name] = inputImageViewsSRGB;
                renderImageViewsSRGB[texture.unique_name]  = inputImageViewsSRGB;

                textureFormatsUNORM[texture.unique_name] = inputOutputFormatUNORM;
                textureFormatsSRGB[texture.unique_name]  = inputOutputFormatSRGB;
                continue;
            }

            const VkExtent3D textureExtent = {.width = texture.width, .height = texture.height, .depth = 1};
            // TODO handle mip map levels correctly
            // TODO handle pooled textures better
            const auto source = std::ranges::find(texture.annotations, "source", &reshadefx::annotation::name);

            if (source == std::cend(texture.annotations))
            {
                textureMemory.emplace_back(VK_NULL_HANDLE);
                VkImage image = createImage(pLogicalDevice,
                                            textureExtent,
                                            convertReshadeFormat(texture.format),
                                            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                                                | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                            textureMemory.back(),
                                            texture.levels);

                textureImages[texture.unique_name] = {image};
                const std::vector<VkImageView> imageViewsUNORM(std::size(inputImages),
                                                               createImageView(pLogicalDevice,
                                                                               convertToUNORM(convertReshadeFormat(texture.format)),
                                                                               image,
                                                                               VK_IMAGE_VIEW_TYPE_2D,
                                                                               VK_IMAGE_ASPECT_COLOR_BIT,
                                                                               texture.levels));

                const std::vector<VkImageView> imageViewsSRGB(std::size(inputImages),
                                                              createImageView(pLogicalDevice,
                                                                              convertToSRGB(convertReshadeFormat(texture.format)),
                                                                              image,
                                                                              VK_IMAGE_VIEW_TYPE_2D,
                                                                              VK_IMAGE_ASPECT_COLOR_BIT,
                                                                              texture.levels));

                textureImageViewsUNORM[texture.unique_name] = imageViewsUNORM;
                textureImageViewsSRGB[texture.unique_name]  = imageViewsSRGB;

                if (texture.levels > 1)
                {

                    renderImageViewsUNORM[texture.unique_name].assign(
                        std::size(inputImages), createImageView(pLogicalDevice, convertToUNORM(convertReshadeFormat(texture.format)), image));

                    renderImageViewsSRGB[texture.unique_name].assign(
                        std::size(inputImages), createImageView(pLogicalDevice, convertToSRGB(convertReshadeFormat(texture.format)), image));
                }
                else
                {
                    renderImageViewsUNORM[texture.unique_name] = imageViewsUNORM;
                    renderImageViewsSRGB[texture.unique_name]  = imageViewsSRGB;
                }

                textureFormatsUNORM[texture.unique_name] = convertToUNORM(convertReshadeFormat(texture.format));
                textureFormatsSRGB[texture.unique_name]  = convertToSRGB(convertReshadeFormat(texture.format));
                changeImageLayout(pLogicalDevice, std::span{std::addressof(image), 1U}, texture.levels);
                continue;
            }

            textureMemory.emplace_back(VK_NULL_HANDLE);
            const auto image = createImage(pLogicalDevice,
                                           textureExtent,
                                           convertReshadeFormat(texture.format), // TODO search for format and save it
                                           VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                           textureMemory.back(),
                                           texture.levels);

            textureImages[texture.unique_name] = {image};

            auto imageViewUniform = createImageView(pLogicalDevice,
                                                    convertToUNORM(convertReshadeFormat(texture.format)),
                                                    image,
                                                    VK_IMAGE_VIEW_TYPE_2D,
                                                    VK_IMAGE_ASPECT_COLOR_BIT,
                                                    texture.levels);

            auto imageViewSRGB = createImageView(pLogicalDevice,
                                                 convertToSRGB(convertReshadeFormat(texture.format)),
                                                 image,
                                                 VK_IMAGE_VIEW_TYPE_2D,
                                                 VK_IMAGE_ASPECT_COLOR_BIT,
                                                 texture.levels);

            textureImageViewsUNORM[texture.unique_name].assign(std::size(inputImages), imageViewUniform);
            textureImageViewsSRGB[texture.unique_name].assign(std::size(inputImages), imageViewSRGB);

            renderImageViewsUNORM[texture.unique_name].assign(std::size(inputImages), imageViewUniform);
            renderImageViewsSRGB[texture.unique_name].assign(std::size(inputImages), imageViewSRGB);

            textureFormatsUNORM[texture.unique_name] = convertToUNORM(convertReshadeFormat(texture.format));
            textureFormatsSRGB[texture.unique_name]  = convertToSRGB(convertReshadeFormat(texture.format));

            int desiredChannels{};
            switch (textureFormatsUNORM[texture.unique_name])
            {
                case VK_FORMAT_R8_UNORM:
                {
                    desiredChannels = STBI_grey;
                    break;
                }
                case VK_FORMAT_R8G8_UNORM:
                {
                    desiredChannels = STBI_rgb_alpha; // TODO why doesn't STBI_grey_alpha work?
                    break;
                }
                case VK_FORMAT_R8G8B8A8_UNORM:
                {
                    desiredChannels = STBI_rgb_alpha;
                    break;
                }
                case VK_FORMAT_R8G8B8A8_SRGB:
                {
                    desiredChannels = STBI_rgb_alpha;
                    break;
                }
                default:
                {
                    Logger::err("unsupported texture upload format" + std::to_string(textureFormatsUNORM[texture.unique_name]));
                    desiredChannels = STBI_rgb_alpha;
                    break;
                }
            }

            const std::string    filePath = config.getOption<std::string>("reshadeTexturePath") + "/" + source->value.string_data;
            stbi_uc*             pixels{};
            std::vector<stbi_uc> resizedPixels{};
            int                  width{};
            int                  height{};

            auto size = textureExtent.width * textureExtent.height * desiredChannels;

            FILE* const file = std::fopen(filePath.c_str(), "rb");

            if (file == nullptr)
            {
                Logger::err("couldn't open texture: " + filePath);
            }
            if (stbi_dds_test_file(file) != 0)
            {
                int channels{};
                pixels = stbi_dds_load_from_file(file, std::addressof(width), std::addressof(height), std::addressof(channels), desiredChannels);
            }
            else
            {
                int channels{};
                pixels = stbi_load_from_file(file, std::addressof(width), std::addressof(height), std::addressof(channels), desiredChannels);
            }

            // change RGBA to RG
            if (textureFormatsUNORM[texture.unique_name] == VK_FORMAT_R8G8_UNORM)
            {
                uint32_t pos = 0;
                for (uint32_t j = 0; j < size; j += 4)
                {
                    pixels[pos] = pixels[j];
                    ++pos;
                    pixels[pos] = pixels[j + 1];
                    ++pos;
                }
                size /= 2;
                desiredChannels /= 2;
            }

            if (std::cmp_not_equal(width, textureExtent.width) || std::cmp_not_equal(height, textureExtent.height))
            {
                resizedPixels.resize(size);
                stbir_resize_uint8(pixels, width, height, 0, std::data(resizedPixels), textureExtent.width, textureExtent.height, 0, desiredChannels);
            }

            uploadToImage(pLogicalDevice, image, textureExtent, size, std::empty(resizedPixels) ? pixels : std::data(resizedPixels), texture.levels);
            stbi_image_free(pixels);
        }

        for (auto& info : module.samplers)
        {
            samplers.emplace_back(createReshadeSampler(pLogicalDevice, info));
            imageViewVector.emplace_back((info.srgb != 0U) ? textureImageViewsSRGB[info.texture_name] : textureImageViewsUNORM[info.texture_name]);
        }

        uniformDescriptorSetLayout      = createUniformBufferDescriptorSetLayout(pLogicalDevice);
        imageSamplerDescriptorSetLayout = createImageSamplerDescriptorSetLayout(pLogicalDevice, std::size(module.samplers));
        Logger::debug("created descriptorSetLayouts");

        const VkDescriptorPoolSize imagePoolSize{.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                 .descriptorCount = static_cast<uint32_t>(std::size(inputImages) * std::size(module.samplers) * 3)};

        const VkDescriptorPoolSize bufferPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 3};

        const std::array poolSizes{imagePoolSize, bufferPoolSize};

        descriptorPool = createDescriptorPool(pLogicalDevice, poolSizes);
        Logger::debug("created descriptorPool");

        const std::array descriptorSetLayouts{uniformDescriptorSetLayout, imageSamplerDescriptorSetLayout};
        pipelineLayout = createGraphicsPipelineLayout(pLogicalDevice, descriptorSetLayouts);

        Logger::debug("created Pipeline layout");

        Logger::debug("output writes: " + std::to_string(outputWrites));
        if (bufferSize != 0U)
        {
            bufferDescriptorSet = writeBufferDescriptorSet(pLogicalDevice, descriptorPool, uniformDescriptorSetLayout, stagingBuffer);
        }

        inputDescriptorSets =
            allocateAndWriteImageSamplerDescriptorSets(pLogicalDevice, descriptorPool, imageSamplerDescriptorSetLayout, samplers, imageViewVector);

        // count the back buffer writes
        for (auto& pass : module.techniques.front().passes)
        {
            if (std::empty(pass.render_target_names[0]))
            {
                ++outputWrites;
            }
        }

        // if there is only one outputWrite, we can directly write to outputImages
        if (outputWrites > 1)
        {
            textureMemory.emplace_back(VK_NULL_HANDLE);
            backBufferImages = createImages(pLogicalDevice,
                                            std::size(inputImages),
                                            {.width = imageExtent.width, .height = imageExtent.height, .depth = 1},
                                            format, // TODO search for format and save it
                                            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                            textureMemory.back());

            backBufferImageViewsSRGB  = createImageViews(pLogicalDevice, inputOutputFormatSRGB, backBufferImages);
            backBufferImageViewsUNORM = createImageViews(pLogicalDevice, inputOutputFormatUNORM, backBufferImages);

            std::ranges::replace(imageViewVector, inputImageViewsSRGB, backBufferImageViewsSRGB);
            std::ranges::replace(imageViewVector, inputImageViewsUNORM, backBufferImageViewsUNORM);

            backBufferDescriptorSets = allocateAndWriteImageSamplerDescriptorSets(
                pLogicalDevice, descriptorPool, imageSamplerDescriptorSetLayout, samplers, imageViewVector);
        }
        if (outputWrites > 2)
        {
            std::ranges::replace(imageViewVector, backBufferImageViewsSRGB, outputImageViewsSRGB);
            std::ranges::replace(imageViewVector, backBufferImageViewsUNORM, outputImageViewsUNORM);
            outputDescriptorSets = allocateAndWriteImageSamplerDescriptorSets(
                pLogicalDevice, descriptorPool, imageSamplerDescriptorSetLayout, samplers, imageViewVector);
        }

        Logger::debug("after writing ImageSamplerDescriptorSets");

        bool firstTimeStencilAccess = true; // Used to clear the sttencil attachment on the first time
        for (bool outputToBackBuffer = outputWrites % 2 == 0; auto& pass : module.techniques[0].passes)
        {
            std::vector<VkAttachmentReference>               attachmentReferences;
            std::vector<VkAttachmentDescription>             attachmentDescriptions;
            std::vector<VkPipelineColorBlendAttachmentState> attachmentBlendStates;
            std::vector<std::vector<VkImageView>>            attachmentImageViews;
            std::vector<std::string>                         currentRenderTargets;

            for (int i = 0; i < std::size(pass.render_target_names); ++i)
            {
                const auto& target = pass.render_target_names[i];
                Logger::debug("render target:" + target);

                VkAttachmentDescription attachmentDescription{.flags          = 0,
                                                              .format         = (pass.srgb_write_enable != 0U) ? textureFormatsSRGB[target]
                                                                                                               : textureFormatsUNORM[target],
                                                              .samples        = VK_SAMPLE_COUNT_1_BIT,
                                                              .loadOp         = (pass.clear_render_targets != 0U) ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                                                                : (pass.blend_enable != 0U)       ? VK_ATTACHMENT_LOAD_OP_LOAD
                                                                                                                  : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                                              .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                                                              .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                                              .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                                              .initialLayout  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                              .finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                if (std::empty(target))
                {
                    if (i == 0)
                    {
                        break;
                    }
                    attachmentDescription.format        = (pass.srgb_write_enable != 0U) ? inputOutputFormatSRGB : inputOutputFormatUNORM;
                    attachmentDescription.loadOp        = VK_ATTACHMENT_LOAD_OP_LOAD;
                    attachmentDescription.storeOp       = VK_ATTACHMENT_STORE_OP_STORE;
                    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    attachmentDescription.finalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                }

                attachmentDescriptions.emplace_back(attachmentDescription);

                attachmentReferences.emplace_back(
                    VkAttachmentReference{.attachment = static_cast<uint32_t>(i), .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

                const VkPipelineColorBlendAttachmentState colorBlendAttachment{.blendEnable         = pass.blend_enable,
                                                                               .srcColorBlendFactor = convertReshadeBlendFactor(pass.src_blend),
                                                                               .dstColorBlendFactor = convertReshadeBlendFactor(pass.dest_blend),
                                                                               .colorBlendOp        = convertReshadeBlendOp(pass.blend_op),
                                                                               .srcAlphaBlendFactor = convertReshadeBlendFactor(pass.src_blend_alpha),
                                                                               .dstAlphaBlendFactor =
                                                                                   convertReshadeBlendFactor(pass.dest_blend_alpha),
                                                                               .alphaBlendOp   = convertReshadeBlendOp(pass.blend_op_alpha),
                                                                               .colorWriteMask = pass.color_write_mask};

                attachmentBlendStates.emplace_back(colorBlendAttachment);

                attachmentImageViews.emplace_back((pass.srgb_write_enable != 0U) ? renderImageViewsSRGB[target] : renderImageViewsUNORM[target]);
                if (not std::empty(target))
                {
                    currentRenderTargets.emplace_back(target);
                }
            }

            renderTargets.emplace_back(currentRenderTargets);

            const VkRect2D scissor{.offset = {.x = 0, .y = 0},
                                   .extent = {.width  = (pass.viewport_width != 0U) ? pass.viewport_width : imageExtent.width,
                                              .height = (pass.viewport_height != 0U) ? pass.viewport_height : imageExtent.height}};

            Logger::debug(std::to_string(scissor.extent.width) + " x " + std::to_string(scissor.extent.height));

            const VkViewport viewport{.x        = 0.0F,
                                      .y        = 0.0F,
                                      .width    = static_cast<float>(scissor.extent.width),
                                      .height   = static_cast<float>(scissor.extent.height),
                                      .minDepth = 0.0F,
                                      .maxDepth = 1.0F};

            uint32_t depthAttachmentCount = 0;

            if (scissor.extent.width == imageExtent.width && scissor.extent.height == imageExtent.height)
            {
                depthAttachmentCount = 1;

                attachmentImageViews.emplace_back(std::size(inputImages), stencilImageView);

                const VkAttachmentReference attachmentReference{.attachment = static_cast<uint32_t>(std::size(attachmentReferences)),
                                                                .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

                attachmentReferences.emplace_back(attachmentReference);

                const VkAttachmentDescription attachmentDescription{.flags          = 0,
                                                                    .format         = stencilFormat,
                                                                    .samples        = VK_SAMPLE_COUNT_1_BIT,
                                                                    .loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                                                    .storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                                                    .stencilLoadOp  = firstTimeStencilAccess ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                                                                                             : VK_ATTACHMENT_LOAD_OP_LOAD,
                                                                    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
                                                                    .initialLayout  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                                    .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

                firstTimeStencilAccess = false;

                attachmentDescriptions.emplace_back(attachmentDescription);
            }

            // renderpass

            const VkSubpassDescription subpassDescription{
                .flags                   = 0,
                .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .inputAttachmentCount    = 0,
                .pInputAttachments       = nullptr,
                .colorAttachmentCount    = static_cast<uint32_t>(std::size(attachmentReferences) - depthAttachmentCount),
                .pColorAttachments       = std::data(attachmentReferences),
                .pResolveAttachments     = nullptr,
                .pDepthStencilAttachment = (depthAttachmentCount != 0U) ? std::addressof(attachmentReferences.back()) : nullptr,
                .preserveAttachmentCount = 0,
                .pPreserveAttachments    = nullptr};

            const VkSubpassDependency subpassDependency{.srcSubpass      = VK_SUBPASS_EXTERNAL,
                                                        .dstSubpass      = 0,
                                                        .srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                        .dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                        .srcAccessMask   = 0,
                                                        .dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                                        .dependencyFlags = 0};

            const VkRenderPassCreateInfo renderPassCreateInfo{.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                                                              .pNext           = nullptr,
                                                              .flags           = 0,
                                                              .attachmentCount = static_cast<uint32_t>(std::size(attachmentDescriptions)),
                                                              .pAttachments    = std::data(attachmentDescriptions),
                                                              .subpassCount    = 1,
                                                              .pSubpasses      = std::addressof(subpassDescription),
                                                              .dependencyCount = 1,
                                                              .pDependencies   = std::addressof(subpassDependency)};

            VkRenderPass renderPass{nullptr};
            VkResult     result = pLogicalDevice->vkd.CreateRenderPass(
                pLogicalDevice->device, std::addressof(renderPassCreateInfo), nullptr, std::addressof(renderPass));
            AssertVulkan(result);
            renderPasses.emplace_back(renderPass);

            std::vector<VkClearValue>   clearValues(std::size(attachmentDescriptions)); // TODO: why it was size of 9 before???
            const VkRenderPassBeginInfo renderPassBeginInfo{.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                                            .pNext           = nullptr,
                                                            .renderPass      = renderPass,
                                                            .framebuffer     = VK_NULL_HANDLE, // changed at apply time
                                                            .renderArea      = scissor,
                                                            .clearValueCount = static_cast<uint32_t>(std::size(clearValues)),
                                                            .pClearValues    = std::data(clearValues)};

            renderPassBeginInfos.emplace_back(renderPassBeginInfo);

            // framebuffers

            if (std::empty(pass.render_target_names[0]))
            {
                const auto& backBufferImageViews = (pass.srgb_write_enable != 0U) ? backBufferImageViewsSRGB : backBufferImageViewsUNORM;
                const auto& outputImageViews     = (pass.srgb_write_enable != 0U) ? outputImageViewsSRGB : outputImageViewsUNORM;
                framebuffers.emplace_back(createFramebuffers(pLogicalDevice,
                                                             renderPass,
                                                             imageExtent,
                                                             {outputToBackBuffer ? backBufferImageViews : outputImageViews,
                                                              std::vector<VkImageView>(std::size(inputImages), stencilImageView)}));
                outputToBackBuffer = !outputToBackBuffer;
                switchSamplers.emplace_back(true);
            }
            else
            {
                framebuffers.emplace_back(createFramebuffers(pLogicalDevice, renderPass, scissor.extent, attachmentImageViews));
                switchSamplers.emplace_back(false);
            }

            // pipeline

            // Configure effect
            std::vector<VkSpecializationMapEntry> specMapEntrys;
            std::vector<char>                     specData;

            for (uint32_t specId = 0, offset = 0; auto& opt : module.spec_constants)
            {
                if (not std::empty(opt.name))
                {
                    auto val = config.getOption<std::string>(opt.name);
                    if (not std::empty(val))
                    {
                        std::variant<int32_t, uint32_t, float> convertedValue;
                        offset = static_cast<uint32_t>(std::size(specData));
                        switch (opt.type.base)
                        {
                            case reshadefx::type::t_bool:
                                convertedValue.emplace<int32_t>(static_cast<int32_t>(config.getOption<bool>(opt.name)));
                                specData.resize(offset + sizeof(VkBool32));
                                std::memcpy(std::data(specData) + offset, std::addressof(convertedValue), sizeof(VkBool32));
                                specMapEntrys.emplace_back(specId, offset, sizeof(VkBool32));
                                break;
                            case reshadefx::type::t_int:
                                convertedValue.emplace<int32_t>(config.getOption<int32_t>(opt.name));
                                specData.resize(offset + sizeof(int32_t));
                                std::memcpy(std::data(specData) + offset, std::addressof(convertedValue), sizeof(int32_t));
                                specMapEntrys.emplace_back(specId, offset, sizeof(int32_t));
                                break;
                            case reshadefx::type::t_uint:
                                convertedValue.emplace<uint32_t>(config.getOption<int32_t>(opt.name));
                                specData.resize(offset + sizeof(uint32_t));
                                std::memcpy(std::data(specData) + offset, std::addressof(convertedValue), sizeof(uint32_t));
                                specMapEntrys.emplace_back(specId, offset, sizeof(uint32_t));
                                break;
                            case reshadefx::type::t_float:
                                convertedValue.emplace<float>(config.getOption<float>(opt.name));
                                specData.resize(offset + sizeof(float));
                                std::memcpy(std::data(specData) + offset, std::addressof(convertedValue), sizeof(float));
                                specMapEntrys.emplace_back(specId, offset, sizeof(float));
                                break;
                            default:
                                // do nothing
                                break;
                        }
                    }
                }
                ++specId;
            }

            VkSpecializationInfo specializationInfo;
            if (not std::empty(specMapEntrys))
            {
                specializationInfo = {.mapEntryCount = static_cast<uint32_t>(std::size(specMapEntrys)),
                                      .pMapEntries   = std::data(specMapEntrys),
                                      .dataSize      = std::size(specData),
                                      .pData         = std::data(specData)};
            }

            const VkPipelineShaderStageCreateInfo shaderStageCreateInfoVert{
                .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext               = nullptr,
                .flags               = 0,
                .stage               = VK_SHADER_STAGE_VERTEX_BIT,
                .module              = shaderModule,
                .pName               = pass.vs_entry_point.c_str(),
                .pSpecializationInfo = (std::empty(specMapEntrys)) ? nullptr : std::addressof(specializationInfo)};

            const VkPipelineShaderStageCreateInfo shaderStageCreateInfoFrag{
                .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext               = nullptr,
                .flags               = 0,
                .stage               = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module              = shaderModule,
                .pName               = pass.ps_entry_point.c_str(),
                .pSpecializationInfo = (std::empty(specMapEntrys)) ? nullptr : std::addressof(specializationInfo)};

            const std::array shaderStages{shaderStageCreateInfoVert, shaderStageCreateInfoFrag};

            VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                                                                       .pNext = nullptr,
                                                                       .flags = 0,
                                                                       .vertexBindingDescriptionCount   = 0,
                                                                       .pVertexBindingDescriptions      = nullptr,
                                                                       .vertexAttributeDescriptionCount = 0,
                                                                       .pVertexAttributeDescriptions    = nullptr};

            VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

            switch (pass.topology)
            {
                case reshadefx::primitive_topology::point_list: topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST; break;
                case reshadefx::primitive_topology::line_list: topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; break;
                case reshadefx::primitive_topology::line_strip: topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP; break;
                case reshadefx::primitive_topology::triangle_list: topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; break;
                case reshadefx::primitive_topology::triangle_strip: topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; break;
                default: Logger::err("unsupported primitiv type" + convertToString(static_cast<uint8_t>(pass.topology))); break;
            }

            const VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                                                                                 .pNext = nullptr,
                                                                                 .flags = 0,
                                                                                 .topology               = topology,
                                                                                 .primitiveRestartEnable = VK_FALSE};

            const VkPipelineViewportStateCreateInfo viewportStateCreateInfo{.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                                                                            .pNext         = nullptr,
                                                                            .flags         = 0,
                                                                            .viewportCount = 1,
                                                                            .pViewports    = std::addressof(viewport),
                                                                            .scissorCount  = 1,
                                                                            .pScissors     = std::addressof(scissor)};

            const VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                                                                                 .pNext = nullptr,
                                                                                 .flags = 0,
                                                                                 .depthClampEnable        = VK_FALSE,
                                                                                 .rasterizerDiscardEnable = VK_FALSE,
                                                                                 .polygonMode             = VK_POLYGON_MODE_FILL,
                                                                                 .cullMode                = VK_CULL_MODE_NONE,
                                                                                 .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                                                                                 .depthBiasEnable         = VK_FALSE,
                                                                                 .depthBiasConstantFactor = 0.0F,
                                                                                 .depthBiasClamp          = 0.0F,
                                                                                 .depthBiasSlopeFactor    = 0.0F,
                                                                                 .lineWidth               = 1.0F};

            const VkPipelineMultisampleStateCreateInfo multisampleCreateInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                                                                             .pNext = nullptr,
                                                                             .flags = 0,
                                                                             .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
                                                                             .sampleShadingEnable   = VK_FALSE,
                                                                             .minSampleShading      = 1.0F,
                                                                             .pSampleMask           = nullptr,
                                                                             .alphaToCoverageEnable = VK_FALSE,
                                                                             .alphaToOneEnable      = VK_FALSE};

            const VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo{.sType         = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                                                                           .pNext         = nullptr,
                                                                           .flags         = 0,
                                                                           .logicOpEnable = VK_FALSE,
                                                                           .logicOp       = VK_LOGIC_OP_NO_OP,
                                                                           .attachmentCount = static_cast<uint32_t>(std::size(attachmentBlendStates)),
                                                                           .pAttachments    = std::data(attachmentBlendStates),
                                                                           .blendConstants  = {0.0F, 0.0F, 0.0F, 0.0F}};

            const VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                                                                          .pNext             = nullptr,
                                                                          .flags             = 0,
                                                                          .dynamicStateCount = 0,
                                                                          .pDynamicStates    = nullptr};

            const VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {
                .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                .pNext                 = nullptr,
                .depthTestEnable       = VK_FALSE,
                .depthWriteEnable      = VK_FALSE,
                .depthCompareOp        = VK_COMPARE_OP_ALWAYS,
                .depthBoundsTestEnable = VK_FALSE,
                .stencilTestEnable     = pass.stencil_enable,
                .front                 = {.failOp      = convertReshadeStencilOp(pass.stencil_op_fail),
                                          .passOp      = convertReshadeStencilOp(pass.stencil_op_pass),
                                          .depthFailOp = convertReshadeStencilOp(pass.stencil_op_depth_fail),
                                          .compareOp   = convertReshadeCompareOp(pass.stencil_comparison_func),
                                          .compareMask = pass.stencil_read_mask,
                                          .writeMask   = pass.stencil_write_mask,
                                          .reference   = pass.stencil_reference_value},
                .back                  = depthStencilStateCreateInfo.front,
                .minDepthBounds        = 0.0F,
                .maxDepthBounds        = 1.0F};

            const VkGraphicsPipelineCreateInfo pipelineCreateInfo{.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                                                                  .pNext               = nullptr,
                                                                  .flags               = 0,
                                                                  .stageCount          = std::size(shaderStages),
                                                                  .pStages             = std::data(shaderStages),
                                                                  .pVertexInputState   = std::addressof(vertexInputCreateInfo),
                                                                  .pInputAssemblyState = std::addressof(inputAssemblyCreateInfo),
                                                                  .pTessellationState  = nullptr,
                                                                  .pViewportState      = std::addressof(viewportStateCreateInfo),
                                                                  .pRasterizationState = std::addressof(rasterizationCreateInfo),
                                                                  .pMultisampleState   = std::addressof(multisampleCreateInfo),
                                                                  .pDepthStencilState  = std::addressof(depthStencilStateCreateInfo),
                                                                  .pColorBlendState    = std::addressof(colorBlendCreateInfo),
                                                                  .pDynamicState       = std::addressof(dynamicStateCreateInfo),
                                                                  .layout              = pipelineLayout,
                                                                  .renderPass          = renderPass,
                                                                  .subpass             = 0,
                                                                  .basePipelineHandle  = VK_NULL_HANDLE,
                                                                  .basePipelineIndex   = -1};

            VkPipeline pipeline{};
            result = pLogicalDevice->vkd.CreateGraphicsPipelines(
                pLogicalDevice->device, VK_NULL_HANDLE, 1, std::addressof(pipelineCreateInfo), nullptr, std::addressof(pipeline));
            AssertVulkan(result);
            graphicsPipelines.emplace_back(pipeline);

            Logger::debug("vertex   entry: " + pass.vs_entry_point);
            Logger::debug("fragment entry: " + pass.ps_entry_point);
        }
        Logger::debug("finished creating Reshade effect");
    }

    void ReshadeEffect::updateEffect()
    {
        if (bufferSize != 0U)
        {
            void*    data{};
            const auto result = pLogicalDevice->vkd.MapMemory(pLogicalDevice->device, stagingBufferMemory, 0, bufferSize, 0, std::addressof(data));
            AssertVulkan(result);
            for (auto& uniform : uniforms)
            {
                uniform->update(data);
            }
            pLogicalDevice->vkd.UnmapMemory(pLogicalDevice->device, stagingBufferMemory);
        }
    }

    void ReshadeEffect::useDepthImage(VkImageView depthImageView)
    {
        std::vector<std::string> depthTextureNames;

        for (auto& texture : module.textures)
        {
            if (texture.semantic == "DEPTH")
            {
                depthTextureNames.emplace_back(texture.unique_name);
            }
        }

        for (auto [infoIndex, info] : module.samplers | std::views::enumerate)
        {
            if (std::ranges::contains(depthTextureNames, info.texture_name))
            {
                for (uint32_t j = 0; j < std::size(inputImages); ++j)
                {
                    const VkDescriptorImageInfo imageInfo{
                        .sampler     = samplers[infoIndex],
                        .imageView   = (depthImageView != nullptr)
                                           ? depthImageView
                                           : inputImageViewsUNORM[j], // Use a input image if there is no depth image to prevent a crash
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

                    VkWriteDescriptorSet writeDescriptorSet{.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                                            .pNext            = nullptr,
                                                            .dstSet           = inputDescriptorSets[j],
                                                            .dstBinding       = static_cast<uint32_t>(infoIndex),
                                                            .dstArrayElement  = 0,
                                                            .descriptorCount  = 1,
                                                            .descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                            .pImageInfo       = std::addressof(imageInfo),
                                                            .pBufferInfo      = nullptr,
                                                            .pTexelBufferView = nullptr};

                    pLogicalDevice->vkd.UpdateDescriptorSets(pLogicalDevice->device, 1, std::addressof(writeDescriptorSet), 0, nullptr);
                    if (outputWrites > 1)
                    {
                        writeDescriptorSet.dstSet = backBufferDescriptorSets[j];
                        pLogicalDevice->vkd.UpdateDescriptorSets(pLogicalDevice->device, 1, std::addressof(writeDescriptorSet), 0, nullptr);
                    }
                    if (outputWrites > 2)
                    {
                        writeDescriptorSet.dstSet = outputDescriptorSets[j];
                        pLogicalDevice->vkd.UpdateDescriptorSets(pLogicalDevice->device, 1, std::addressof(writeDescriptorSet), 0, nullptr);
                    }
                }
            }
        }
    }

    void ReshadeEffect::applyEffect(uint32_t imageIndex, VkCommandBuffer commandBuffer)
    {
        if (std::size(inputImages) <= imageIndex)
        {
            Logger::err("imageIndex is out of range");
            return;
        }

        Logger::debug("applying ReshadeEffect to command buffer" + convertToString(commandBuffer));
        // Used to make the Image accessable by the shader
        VkImageMemoryBarrier memoryBarrier{
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext               = nullptr,
            .srcAccessMask       = VK_ACCESS_MEMORY_WRITE_BIT,
            .dstAccessMask       = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = inputImages[imageIndex],
            .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}};

        // Reverses the first Barrier
        VkImageMemoryBarrier secondBarrier{
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext               = nullptr,
            .srcAccessMask       = VK_ACCESS_SHADER_READ_BIT,
            .dstAccessMask       = 0,
            .oldLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .newLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = inputImages[imageIndex],
            .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}};

        pLogicalDevice->vkd.CmdPipelineBarrier(commandBuffer,
                                               VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                               0,
                                               0,
                                               nullptr,
                                               0,
                                               nullptr,
                                               1,
                                               std::addressof(memoryBarrier));
        memoryBarrier.image     = outputImages[imageIndex];
        memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        memoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        pLogicalDevice->vkd.CmdPipelineBarrier(commandBuffer,
                                               VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                               0,
                                               0,
                                               nullptr,
                                               0,
                                               nullptr,
                                               1,
                                               std::addressof(memoryBarrier));
        if (outputWrites > 1)
        {
            memoryBarrier.image = backBufferImages[imageIndex];
            pLogicalDevice->vkd.CmdPipelineBarrier(commandBuffer,
                                                   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                                   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                                   0,
                                                   0,
                                                   nullptr,
                                                   0,
                                                   nullptr,
                                                   1,
                                                   std::addressof(memoryBarrier));
        }

        // stencil image
        memoryBarrier.image                       = stencilImage;
        memoryBarrier.srcAccessMask               = 0;
        memoryBarrier.dstAccessMask               = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        memoryBarrier.oldLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
        memoryBarrier.newLayout                   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT;

        pLogicalDevice->vkd.CmdPipelineBarrier(commandBuffer,
                                               VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                               VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                                               0,
                                               0,
                                               nullptr,
                                               0,
                                               nullptr,
                                               1,
                                               std::addressof(memoryBarrier));

        Logger::debug("after the first pipeline barrier");

        pLogicalDevice->vkd.CmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, std::addressof(inputDescriptorSets[imageIndex]), 0, nullptr);
        Logger::debug("after binding image sampler");

        if (bufferSize != 0U)
        {
            pLogicalDevice->vkd.CmdBindDescriptorSets(
                commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, std::addressof(bufferDescriptorSet), 0, nullptr);
            Logger::debug("after binding uniform buffer");
        }

        bool backBufferNext = outputWrites % 2 == 0;
        for (size_t i = 0; i < std::size(graphicsPipelines); ++i)
        {
            renderPassBeginInfos[i].framebuffer = framebuffers[i][imageIndex];

            Logger::debug("before beginn renderpass");
            pLogicalDevice->vkd.CmdBeginRenderPass(commandBuffer, std::addressof(renderPassBeginInfos[i]), VK_SUBPASS_CONTENTS_INLINE);
            Logger::debug("after beginn renderpass");

            pLogicalDevice->vkd.CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[i]);
            Logger::debug("after bind pipeliene");

            pLogicalDevice->vkd.CmdDraw(commandBuffer, module.techniques.front().passes[i].num_vertices, 1, 0, 0);
            Logger::debug("after draw");

            pLogicalDevice->vkd.CmdEndRenderPass(commandBuffer);
            Logger::debug("after end renderpass");

            if (switchSamplers[i] && outputWrites > 1)
            {
                if (backBufferNext)
                {
                    pLogicalDevice->vkd.CmdBindDescriptorSets(commandBuffer,
                                                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                              pipelineLayout,
                                                              1,
                                                              1,
                                                              std::addressof(backBufferDescriptorSets[imageIndex]),
                                                              0,
                                                              nullptr);
                }
                else if (outputWrites > 2)
                {
                    pLogicalDevice->vkd.CmdBindDescriptorSets(commandBuffer,
                                                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                              pipelineLayout,
                                                              1,
                                                              1,
                                                              std::addressof(outputDescriptorSets[imageIndex]),
                                                              0,
                                                              nullptr);
                }
                backBufferNext = !backBufferNext;
            }

            for (auto& renderTarget : renderTargets[i])
            {
                generateMipMaps(
                    pLogicalDevice, commandBuffer, textureImages[renderTarget].front(), textureExtents[renderTarget], textureMipLevels[renderTarget]);
            }
        }
        pLogicalDevice->vkd.CmdPipelineBarrier(commandBuffer,
                                               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                               0,
                                               0,
                                               nullptr,
                                               0,
                                               nullptr,
                                               1,
                                               std::addressof(secondBarrier));
        secondBarrier.image = outputImages[imageIndex];
        pLogicalDevice->vkd.CmdPipelineBarrier(commandBuffer,
                                               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                               0,
                                               0,
                                               nullptr,
                                               0,
                                               nullptr,
                                               1,
                                               std::addressof(secondBarrier));
        Logger::debug("after the second pipeline barrier");
    }

    ReshadeEffect::~ReshadeEffect()
    {
        Logger::debug("destroying ReshadeEffect" + convertToString(this));
        for (auto& pipeline : graphicsPipelines)
        {
            pLogicalDevice->vkd.DestroyPipeline(pLogicalDevice->device, pipeline, nullptr);
        }

        if (bufferSize != 0U)
        {
            pLogicalDevice->vkd.FreeMemory(pLogicalDevice->device, stagingBufferMemory, nullptr);
            pLogicalDevice->vkd.DestroyBuffer(pLogicalDevice->device, stagingBuffer, nullptr);
        }

        pLogicalDevice->vkd.DestroyPipelineLayout(pLogicalDevice->device, pipelineLayout, nullptr);
        for (auto& renderPass : renderPasses)
        {
            pLogicalDevice->vkd.DestroyRenderPass(pLogicalDevice->device, renderPass, nullptr);
        }

        pLogicalDevice->vkd.DestroyDescriptorSetLayout(pLogicalDevice->device, imageSamplerDescriptorSetLayout, nullptr);
        pLogicalDevice->vkd.DestroyDescriptorSetLayout(pLogicalDevice->device, uniformDescriptorSetLayout, nullptr);

        pLogicalDevice->vkd.DestroyShaderModule(pLogicalDevice->device, shaderModule, nullptr);

        pLogicalDevice->vkd.DestroyDescriptorPool(pLogicalDevice->device, descriptorPool, nullptr);
        for (auto& imageView : outputImageViewsSRGB)
        {
            pLogicalDevice->vkd.DestroyImageView(pLogicalDevice->device, imageView, nullptr);
        }
        for (auto& imageView : outputImageViewsUNORM)
        {
            pLogicalDevice->vkd.DestroyImageView(pLogicalDevice->device, imageView, nullptr);
        }

        for (auto& imageView : backBufferImageViewsSRGB)
        {
            pLogicalDevice->vkd.DestroyImageView(pLogicalDevice->device, imageView, nullptr);
        }
        for (auto& imageView : backBufferImageViewsUNORM)
        {
            pLogicalDevice->vkd.DestroyImageView(pLogicalDevice->device, imageView, nullptr);
        }

        for (auto& fbs : framebuffers)
        {
            for (auto& fb : fbs)
            {
                pLogicalDevice->vkd.DestroyFramebuffer(pLogicalDevice->device, fb, nullptr);
            }
        }

        std::set<VkImageView> imageViewSet;

        for (auto& [_, textureImageViews] : textureImageViewsSRGB)
        {
            for (auto& imageView : textureImageViews)
            {
                imageViewSet.insert(imageView);
            }
        }
        for (auto& [_, textureImageViews] : textureImageViewsUNORM)
        {
            for (auto& imageView : textureImageViews)
            {
                imageViewSet.insert(imageView);
            }
        }

        for (auto& [_, textureImageViews] : renderImageViewsSRGB)
        {
            for (auto& imageView : textureImageViews)
            {
                imageViewSet.insert(imageView);
            }
        }
        for (auto& [_, textureImageViews] : renderImageViewsUNORM)
        {
            for (auto imageView : textureImageViews)
            {
                imageViewSet.insert(imageView);
            }
        }

        for (const auto& imageView : imageViewSet)
        {
            pLogicalDevice->vkd.DestroyImageView(pLogicalDevice->device, imageView, nullptr);
        }
        pLogicalDevice->vkd.DestroyImageView(pLogicalDevice->device, stencilImageView, nullptr);

        for (auto& [_, textureImage] : textureImages)
        {
            for (auto& image : textureImage)
            {
                pLogicalDevice->vkd.DestroyImage(pLogicalDevice->device, image, nullptr);
            }
        }

        for (auto& image : backBufferImages)
        {
            pLogicalDevice->vkd.DestroyImage(pLogicalDevice->device, image, nullptr);
        }

        pLogicalDevice->vkd.DestroyImage(pLogicalDevice->device, stencilImage, nullptr);

        for (auto& sampler : samplers)
        {
            pLogicalDevice->vkd.DestroySampler(pLogicalDevice->device, sampler, nullptr);
        }

        for (auto& memory : textureMemory)
        {
            pLogicalDevice->vkd.FreeMemory(pLogicalDevice->device, memory, nullptr);
        }
    }

    void ReshadeEffect::createReshadeModule(const Config& config)
    {
        reshadefx::preprocessor preprocessor;
        preprocessor.add_macro_definition("__RESHADE__", std::to_string(INT_MAX));
        preprocessor.add_macro_definition("__RESHADE_PERFORMANCE_MODE__", "1");
        preprocessor.add_macro_definition("__RENDERER__", "0x20000");
        // TODO add more macros

        preprocessor.add_macro_definition("BUFFER_WIDTH", std::to_string(imageExtent.width));
        preprocessor.add_macro_definition("BUFFER_HEIGHT", std::to_string(imageExtent.height));
        preprocessor.add_macro_definition("BUFFER_RCP_WIDTH", "(1.0 / BUFFER_WIDTH)");
        preprocessor.add_macro_definition("BUFFER_RCP_HEIGHT", "(1.0 / BUFFER_HEIGHT)");
        preprocessor.add_macro_definition("BUFFER_COLOR_DEPTH", (inputOutputFormatUNORM == VK_FORMAT_A2R10G10B10_UNORM_PACK32) ? "10" : "8");
        preprocessor.add_include_path(config.getOption<std::string>("reshadeIncludePath"));
        if (!preprocessor.append_file(config.getOption<std::string>(effectName)))
        {
            Logger::err("failed to load shader file: " + config.getOption<std::string>(effectName));
            Logger::err("Does the filepath exist and does it not include spaces?");
        }

        reshadefx::parser parser;

        if (const auto& errors = preprocessor.errors(); not std::empty(errors))
        {
            Logger::err(errors);
        }

        std::unique_ptr<reshadefx::codegen> codegen(reshadefx::create_codegen_spirv(
            true /* vulkan semantics */, true /* debug info */, true /* uniforms to spec constants */, true /*flip vertex shader*/));
        parser.parse(std::move(preprocessor.output()), codegen.get());

        if (const auto& errors = parser.errors(); not std::empty(errors))
        {
            Logger::err(errors);
        }
        codegen->write_result(module);

        const VkShaderModuleCreateInfo shaderCreateInfo{.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                                        .pNext    = nullptr,
                                                        .flags    = 0,
                                                        .codeSize = std::span{module.spirv}.size_bytes(),
                                                        .pCode    = std::data(module.spirv)};

        const auto result =
            pLogicalDevice->vkd.CreateShaderModule(pLogicalDevice->device, std::addressof(shaderCreateInfo), nullptr, std::addressof(shaderModule));
        AssertVulkan(result);

        Logger::debug("created reshade shaderModule");
    }

} // namespace vkBasalt
