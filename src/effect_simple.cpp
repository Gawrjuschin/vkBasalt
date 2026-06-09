#include "effect_simple.hpp"
#include "config.hpp"
#include "image_view.hpp"
#include "descriptor_set.hpp"
#include "logical_device.hpp"
#include "renderpass.hpp"
#include "graphics_pipeline.hpp"
#include "framebuffer.hpp"
#include "shader.hpp"
#include "sampler.hpp"
#include "util.hpp"

#include <cstdint>
#include <iterator>
#include <memory>
#include <ranges>
#include <span>
#include <vector>

#include <logger.hpp>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    SimpleEffect::SimpleEffect() = default;

    void SimpleEffect::init(LogicalDevice*           pLogicalDevice,
                            VkFormat                 format,
                            VkExtent2D               imageExtent,
                            std::span<const VkImage> inputImages,
                            std::span<const VkImage> outputImages,
                            Config*                  pConfig)
    {
        Logger::debug("in creating SimpleEffect");

        this->pLogicalDevice = pLogicalDevice;
        this->format         = format;
        this->imageExtent    = imageExtent;
        this->inputImages.assign(std::cbegin(inputImages), std::cend(inputImages));
        this->outputImages.assign(std::cbegin(outputImages), std::cend(outputImages));
        this->pConfig        = pConfig;

        inputImageViews = createImageViews(pLogicalDevice, format, inputImages);
        Logger::debug("created input ImageViews");
        outputImageViews = createImageViews(pLogicalDevice, format, outputImages);
        Logger::debug("created ImageViews");
        sampler = createSampler(pLogicalDevice);
        Logger::debug("created sampler");

        imageSamplerDescriptorSetLayout = createImageSamplerDescriptorSetLayout(pLogicalDevice, 1);
        Logger::debug("created descriptorSetLayouts");

        const VkDescriptorPoolSize imagePoolSize{.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                 .descriptorCount = static_cast<uint32_t>(std::size(inputImages) + 10)};

        descriptorPool = createDescriptorPool(pLogicalDevice, std::span{std::addressof(imagePoolSize), 1U});
        Logger::debug("created descriptorPool");

        createShaderModule(pLogicalDevice, vertexCode, std::addressof(vertexModule));
        createShaderModule(pLogicalDevice, fragmentCode, std::addressof(fragmentModule));

        renderPass = createRenderPass(pLogicalDevice, format);

        descriptorSetLayouts.insert(descriptorSetLayouts.begin(), imageSamplerDescriptorSetLayout);
        pipelineLayout = createGraphicsPipelineLayout(pLogicalDevice, descriptorSetLayouts);

        graphicsPipeline = createGraphicsPipeline(pLogicalDevice,
                                                  vertexModule,
                                                  pVertexSpecInfo,
                                                  "main",
                                                  fragmentModule,
                                                  pFragmentSpecInfo,
                                                  "main",
                                                  imageExtent,
                                                  renderPass,
                                                  pipelineLayout);

        imageDescriptorSets = allocateAndWriteImageSamplerDescriptorSets(
            pLogicalDevice, descriptorPool, imageSamplerDescriptorSetLayout, {sampler}, std::vector<std::vector<VkImageView>>(1, inputImageViews));

        framebuffers = createFramebuffers(pLogicalDevice, renderPass, imageExtent, {outputImageViews});
    }

    void SimpleEffect::applyEffect(uint32_t imageIndex, VkCommandBuffer commandBuffer)
    {
        if (std::size(inputImages) <= imageIndex)
        {
            Logger::err("imageIndex is out of range");
            return;
        }

        Logger::debug("applying SimpleEffect to cb " + convertToString(commandBuffer));
        // Used to make the Image accessable by the shader
        const VkImageMemoryBarrier memoryBarrier{
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
        const VkImageMemoryBarrier secondBarrier{
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext               = nullptr,
            .srcAccessMask       = VK_ACCESS_SHADER_READ_BIT,
            .dstAccessMask       = 0,
            .oldLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .newLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = inputImages[imageIndex],
            .subresourceRange    = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .layerCount = 1}};

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
        Logger::debug("after the first pipeline barrier");

        const VkClearValue          clearValue = {{{0.0F, 0.0F, 0.0F, 1.0F}}};
        const VkRenderPassBeginInfo renderPassBeginInfo{.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                                        .pNext           = nullptr,
                                                        .renderPass      = renderPass,
                                                        .framebuffer     = framebuffers[imageIndex],
                                                        .renderArea      = {.offset = {.x = 0, .y = 0}, .extent = imageExtent},
                                                        .clearValueCount = 1,
                                                        .pClearValues    = std::addressof(clearValue)};

        Logger::debug("before beginn renderpass");
        pLogicalDevice->vkd.CmdBeginRenderPass(commandBuffer, std::addressof(renderPassBeginInfo), VK_SUBPASS_CONTENTS_INLINE);
        Logger::debug("after beginn renderpass");

        pLogicalDevice->vkd.CmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, std::addressof(imageDescriptorSets[imageIndex]), 0, nullptr);
        Logger::debug("after binding image sampler");

        pLogicalDevice->vkd.CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        Logger::debug("after bind pipeliene");

        pLogicalDevice->vkd.CmdDraw(commandBuffer, 3, 1, 0, 0);
        Logger::debug("after draw");

        pLogicalDevice->vkd.CmdEndRenderPass(commandBuffer);
        Logger::debug("after end renderpass");

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

    SimpleEffect::~SimpleEffect()
    {
        Logger::debug("destroying SimpleEffect " + convertToString(this));
        pLogicalDevice->vkd.DestroyPipeline(pLogicalDevice->device, graphicsPipeline, nullptr);
        pLogicalDevice->vkd.DestroyPipelineLayout(pLogicalDevice->device, pipelineLayout, nullptr);
        pLogicalDevice->vkd.DestroyRenderPass(pLogicalDevice->device, renderPass, nullptr);
        pLogicalDevice->vkd.DestroyDescriptorSetLayout(pLogicalDevice->device, imageSamplerDescriptorSetLayout, nullptr);
        pLogicalDevice->vkd.DestroyShaderModule(pLogicalDevice->device, vertexModule, nullptr);
        pLogicalDevice->vkd.DestroyShaderModule(pLogicalDevice->device, fragmentModule, nullptr);
        pLogicalDevice->vkd.DestroyDescriptorPool(pLogicalDevice->device, descriptorPool, nullptr);

        for (auto [framebuffer, inputImageView, outputImageView] : std::views::zip(framebuffers, inputImageViews, outputImageViews))
        {
            pLogicalDevice->vkd.DestroyFramebuffer(pLogicalDevice->device, framebuffer, nullptr);
            pLogicalDevice->vkd.DestroyImageView(pLogicalDevice->device, inputImageView, nullptr);
            pLogicalDevice->vkd.DestroyImageView(pLogicalDevice->device, outputImageView, nullptr);
        }
        Logger::debug("after DestroyImageView");
        pLogicalDevice->vkd.DestroySampler(pLogicalDevice->device, sampler, nullptr);
    }

} // namespace vkBasalt
