#include "effect_smaa.hpp"
#include "config.hpp"
#include "image_view.hpp"
#include "descriptor_set.hpp"
#include "logical_device.hpp"
#include "renderpass.hpp"
#include "graphics_pipeline.hpp"
#include "framebuffer.hpp"
#include "shader.hpp"
#include "sampler.hpp"
#include "image.hpp"
#include "util.hpp"
#include "shader_sources.hpp"

#include <Textures/AreaTex.h>
#include <Textures/SearchTex.h>

#include <logger.hpp>

#include <iterator>
#include <cstdint>
#include <memory>
#include <ranges>
#include <span>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    namespace
    {

        // get config options
        struct SmaaOptions
        {
            float   screenWidth;
            float   screenHeight;
            float   reverseScreenWidth;
            float   reverseScreenHeight;
            float   threshold;
            int32_t maxSearchSteps;
            int32_t maxSearchStepsDiag;
            int32_t cornerRounding;
        };
    } // namespace

    SmaaEffect::SmaaEffect(LogicalDevice*           pLogicalDevice,
                           VkFormat                 format,
                           VkExtent2D               imageExtent,
                           std::span<const VkImage> inputImages,
                           std::span<const VkImage> outputImages,
                           Config*                  pConfig) :
        pLogicalDevice{pLogicalDevice}, inputImages(std::cbegin(inputImages), std::cend(inputImages)),
        outputImages(std::cbegin(outputImages), std::cend(outputImages)), imageExtent{imageExtent}, format{format}, sampler(createSampler(pLogicalDevice)), pConfig{pConfig}
    {
        Logger::debug("in creating SmaaEffect");

        // create Images for the first and second pass at once -> less memory fragmentation
        const auto edgeAndBlendImages = createImages(pLogicalDevice,
                                                     std::size(inputImages) * 2,
                                                     {.width = imageExtent.width, .height = imageExtent.height, .depth = 1},
                                                     VK_FORMAT_B8G8R8A8_UNORM, // TODO search for format and save it
                                                     VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                     imageMemory);

        edgeImages.assign(std::cbegin(edgeAndBlendImages), std::next(std::cbegin(edgeAndBlendImages), std::size(edgeAndBlendImages) / 2));
        blendImages.assign(std::next(std::cbegin(edgeAndBlendImages), std::size(edgeAndBlendImages) / 2), std::cend(edgeAndBlendImages));

        inputImageViews = createImageViews(pLogicalDevice, format, inputImages);
        Logger::debug("created input ImageViews");
        edgeImageViews = createImageViews(pLogicalDevice, VK_FORMAT_B8G8R8A8_UNORM, edgeImages);
        Logger::debug("created edge  ImageViews");
        blendImageViews = createImageViews(pLogicalDevice, VK_FORMAT_B8G8R8A8_UNORM, blendImages);
        Logger::debug("created blend ImageViews");
        outputImageViews = createImageViews(pLogicalDevice, format, outputImages);
        Logger::debug("created output ImageViews");
        
        Logger::debug("created sampler");

        const VkExtent3D areaImageExtent = {.width = AREATEX_WIDTH, .height = AREATEX_HEIGHT, .depth = 1};

        areaImage = createImage(pLogicalDevice,
                                areaImageExtent,
                                VK_FORMAT_R8G8_UNORM, // TODO search for format and save it
                                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                areaMemory);

        const VkExtent3D searchImageExtent = {.width = SEARCHTEX_WIDTH, .height = SEARCHTEX_HEIGHT, .depth = 1};

        searchImage = createImage(pLogicalDevice,
                                  searchImageExtent,
                                  VK_FORMAT_R8_UNORM, // TODO search for format and save it
                                  VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                  searchMemory);

        uploadToImage(pLogicalDevice, areaImage, areaImageExtent, AREATEX_SIZE, areaTexBytes);

        uploadToImage(pLogicalDevice, searchImage, searchImageExtent, SEARCHTEX_SIZE, searchTexBytes);

        areaImageView = createImageViews(pLogicalDevice, VK_FORMAT_R8G8_UNORM, std::span{std::addressof(areaImage), 1U}).front();
        Logger::debug("after creating area ImageView");
        searchImageView = createImageViews(pLogicalDevice, VK_FORMAT_R8_UNORM, std::span{std::addressof(searchImage), 1U}).front();
        Logger::debug("created search ImageView");

        imageSamplerDescriptorSetLayout = createImageSamplerDescriptorSetLayout(pLogicalDevice, 5);
        Logger::debug("created descriptorSetLayouts");

        VkDescriptorPoolSize imagePoolSize;
        imagePoolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        imagePoolSize.descriptorCount = std::size(inputImages) * 5;

        descriptorPool = createDescriptorPool(pLogicalDevice, std::span{std::addressof(imagePoolSize), 1U});
        Logger::debug("created descriptorPool");

        const SmaaOptions smaaOptions{
            .screenWidth         = static_cast<float>(imageExtent.width),
            .screenHeight        = static_cast<float>(imageExtent.height),
            .reverseScreenWidth  = 1.0F / imageExtent.width,
            .reverseScreenHeight = 1.0F / imageExtent.height,
            .threshold           = pConfig->getOption<float>("smaaThreshold", 0.05F),
            .maxSearchSteps      = pConfig->getOption<int32_t>("smaaMaxSearchSteps", 32),
            .maxSearchStepsDiag  = pConfig->getOption<int32_t>("smaaMaxSearchStepsDiag", 16),
            .cornerRounding      = pConfig->getOption<int32_t>("smaaCornerRounding", 25),
        };

        createShaderModule(pLogicalDevice, smaa_edge_vert, &edgeVertexModule);

        bool useColor = pConfig->getOption<std::string>("smaaEdgeDetection", "luma") == "color";

        useColor ? createShaderModule(pLogicalDevice, smaa_edge_color_frag, std::addressof(edgeFragmentModule))
                 : createShaderModule(pLogicalDevice, smaa_edge_luma_frag, std::addressof(edgeFragmentModule));

        createShaderModule(pLogicalDevice, smaa_blend_vert, &blendVertexModule);

        createShaderModule(pLogicalDevice, smaa_blend_frag, &blendFragmentModule);

        createShaderModule(pLogicalDevice, smaa_neighbor_vert, &neighborVertexModule);

        createShaderModule(pLogicalDevice, smaa_neighbor_frag, &neignborFragmentModule);

        renderPass      = createRenderPass(pLogicalDevice, format);
        unormRenderPass = createRenderPass(pLogicalDevice, VK_FORMAT_B8G8R8A8_UNORM);
        pipelineLayout  = createGraphicsPipelineLayout(pLogicalDevice, std::span{std::addressof(imageSamplerDescriptorSetLayout), 1U});

        constexpr static auto specMapEntrys{[]() {
            std::array<VkSpecializationMapEntry, 8U> specMapEntrys{}; // TODO: why 8
            for (auto [idx, specMapEntry] : specMapEntrys | std::views::enumerate)
            {
                specMapEntrys[idx] = {
                    .constantID = static_cast<uint32_t>(idx),
                    .offset     = static_cast<uint32_t>(sizeof(float) * idx), // TODO not clean to assume that sizeof(int32_t) == sizeof(float)
                    .size       = sizeof(float)};
            }
            return specMapEntrys;
        }()};

        VkSpecializationInfo specializationInfo{.mapEntryCount = std::size(specMapEntrys),
                                                .pMapEntries   = std::data(specMapEntrys),
                                                .dataSize      = sizeof(SmaaOptions),
                                                .pData         = std::addressof(smaaOptions)};

        edgePipeline = createGraphicsPipeline(pLogicalDevice,
                                              edgeVertexModule,
                                              std::addressof(specializationInfo),
                                              "main",
                                              edgeFragmentModule,
                                              std::addressof(specializationInfo),
                                              "main",
                                              imageExtent,
                                              unormRenderPass,
                                              pipelineLayout);

        blendPipeline = createGraphicsPipeline(pLogicalDevice,
                                               blendVertexModule,
                                               std::addressof(specializationInfo),
                                               "main",
                                               blendFragmentModule,
                                               std::addressof(specializationInfo),
                                               "main",
                                               imageExtent,
                                               unormRenderPass,
                                               pipelineLayout);

        neighborPipeline = createGraphicsPipeline(pLogicalDevice,
                                                  neighborVertexModule,
                                                  std::addressof(specializationInfo),
                                                  "main",
                                                  neignborFragmentModule,
                                                  std::addressof(specializationInfo),
                                                  "main",
                                                  imageExtent,
                                                  renderPass,
                                                  pipelineLayout);

        std::vector imageViewsVector = {inputImageViews,
                                        edgeImageViews,
                                        std::vector<VkImageView>(std::size(inputImageViews), areaImageView),
                                        std::vector<VkImageView>(std::size(inputImageViews), searchImageView),
                                        blendImageViews};

        imageDescriptorSets = allocateAndWriteImageSamplerDescriptorSets(pLogicalDevice,
                                                                         descriptorPool,
                                                                         imageSamplerDescriptorSetLayout,
                                                                         std::vector<VkSampler>(std::size(imageViewsVector), sampler),
                                                                         imageViewsVector);

        edgeFramebuffers     = createFramebuffers(pLogicalDevice, unormRenderPass, imageExtent, {edgeImageViews});
        blendFramebuffers    = createFramebuffers(pLogicalDevice, unormRenderPass, imageExtent, {blendImageViews});
        neignborFramebuffers = createFramebuffers(pLogicalDevice, renderPass, imageExtent, {outputImageViews});
    }
    void SmaaEffect::applyEffect(uint32_t imageIndex, VkCommandBuffer commandBuffer)
    {
        Logger::debug("applying smaa effect to cb " + convertToString(commandBuffer));
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
        Logger::debug("after the first pipeline barrier");

        const VkClearValue    clearValue = {0.0F, 0.0F, 0.0F, 1.0F};
        VkRenderPassBeginInfo renderPassBeginInfo{.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                                  .pNext           = nullptr,
                                                  .renderPass      = unormRenderPass,
                                                  .framebuffer     = edgeFramebuffers[imageIndex],
                                                  .renderArea      = {.offset = {0, 0}, .extent = imageExtent},
                                                  .clearValueCount = 1,
                                                  .pClearValues    = std::addressof(clearValue)};
        // edge renderPass
        Logger::debug("before beginn edge renderpass");
        pLogicalDevice->vkd.CmdBeginRenderPass(commandBuffer, std::addressof(renderPassBeginInfo), VK_SUBPASS_CONTENTS_INLINE);
        Logger::debug("after beginn renderpass");

        pLogicalDevice->vkd.CmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, std::addressof(imageDescriptorSets[imageIndex]), 0, nullptr);
        Logger::debug("after binding image sampler");

        pLogicalDevice->vkd.CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, edgePipeline);
        Logger::debug("after bind pipeliene");

        pLogicalDevice->vkd.CmdDraw(commandBuffer, 3, 1, 0, 0);
        Logger::debug("after draw");

        pLogicalDevice->vkd.CmdEndRenderPass(commandBuffer);
        Logger::debug("after end renderpass");

        memoryBarrier.image             = edgeImages[imageIndex];
        renderPassBeginInfo.framebuffer = blendFramebuffers[imageIndex];
        // blend renderPass
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

        Logger::debug("before beginn blend renderpass");
        pLogicalDevice->vkd.CmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        Logger::debug("after beginn renderpass");

        pLogicalDevice->vkd.CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, blendPipeline);
        Logger::debug("after bind pipeliene");

        pLogicalDevice->vkd.CmdDraw(commandBuffer, 3, 1, 0, 0);
        Logger::debug("after draw");

        pLogicalDevice->vkd.CmdEndRenderPass(commandBuffer);
        Logger::debug("after end renderpass");

        memoryBarrier.image             = blendImages[imageIndex];
        renderPassBeginInfo.framebuffer = neignborFramebuffers[imageIndex];
        renderPassBeginInfo.renderPass  = renderPass;
        // neighbor renderPass
        pLogicalDevice->vkd.CmdPipelineBarrier(
            commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &memoryBarrier);
        Logger::debug("after the first pipeline barrier");

        Logger::debug("before beginn neighbor renderpass");
        pLogicalDevice->vkd.CmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        Logger::debug("after beginn renderpass");

        pLogicalDevice->vkd.CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, neighborPipeline);
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
    SmaaEffect::~SmaaEffect()
    {
        Logger::debug("destroying smaa effect " + convertToString(this));
        pLogicalDevice->vkd.DestroyPipeline(pLogicalDevice->device, edgePipeline, nullptr);
        pLogicalDevice->vkd.DestroyPipeline(pLogicalDevice->device, blendPipeline, nullptr);
        pLogicalDevice->vkd.DestroyPipeline(pLogicalDevice->device, neighborPipeline, nullptr);

        pLogicalDevice->vkd.DestroyPipelineLayout(pLogicalDevice->device, pipelineLayout, nullptr);
        pLogicalDevice->vkd.DestroyRenderPass(pLogicalDevice->device, renderPass, nullptr);
        pLogicalDevice->vkd.DestroyRenderPass(pLogicalDevice->device, unormRenderPass, nullptr);
        pLogicalDevice->vkd.DestroyDescriptorSetLayout(pLogicalDevice->device, imageSamplerDescriptorSetLayout, nullptr);

        pLogicalDevice->vkd.DestroyShaderModule(pLogicalDevice->device, edgeVertexModule, nullptr);
        pLogicalDevice->vkd.DestroyShaderModule(pLogicalDevice->device, edgeFragmentModule, nullptr);
        pLogicalDevice->vkd.DestroyShaderModule(pLogicalDevice->device, blendVertexModule, nullptr);
        pLogicalDevice->vkd.DestroyShaderModule(pLogicalDevice->device, blendFragmentModule, nullptr);
        pLogicalDevice->vkd.DestroyShaderModule(pLogicalDevice->device, neighborVertexModule, nullptr);
        pLogicalDevice->vkd.DestroyShaderModule(pLogicalDevice->device, neignborFragmentModule, nullptr);

        pLogicalDevice->vkd.DestroyDescriptorPool(pLogicalDevice->device, descriptorPool, nullptr);
        pLogicalDevice->vkd.FreeMemory(pLogicalDevice->device, imageMemory, nullptr);
        pLogicalDevice->vkd.FreeMemory(pLogicalDevice->device, areaMemory, nullptr);
        pLogicalDevice->vkd.FreeMemory(pLogicalDevice->device, searchMemory, nullptr);

        for (uint32_t i = 0; i < std::size(edgeFramebuffers); ++i)
        {
            pLogicalDevice->vkd.DestroyFramebuffer(pLogicalDevice->device, edgeFramebuffers[i], nullptr);
            pLogicalDevice->vkd.DestroyFramebuffer(pLogicalDevice->device, blendFramebuffers[i], nullptr);
            pLogicalDevice->vkd.DestroyFramebuffer(pLogicalDevice->device, neignborFramebuffers[i], nullptr);
            pLogicalDevice->vkd.DestroyImageView(pLogicalDevice->device, inputImageViews[i], nullptr);
            pLogicalDevice->vkd.DestroyImageView(pLogicalDevice->device, edgeImageViews[i], nullptr);
            pLogicalDevice->vkd.DestroyImageView(pLogicalDevice->device, blendImageViews[i], nullptr);
            pLogicalDevice->vkd.DestroyImageView(pLogicalDevice->device, outputImageViews[i], nullptr);
            pLogicalDevice->vkd.DestroyImage(pLogicalDevice->device, edgeImages[i], nullptr);
            pLogicalDevice->vkd.DestroyImage(pLogicalDevice->device, blendImages[i], nullptr);
        }

        Logger::debug("after DestroyImageView");
        pLogicalDevice->vkd.DestroyImageView(pLogicalDevice->device, areaImageView, nullptr);
        pLogicalDevice->vkd.DestroyImage(pLogicalDevice->device, areaImage, nullptr);
        pLogicalDevice->vkd.DestroyImageView(pLogicalDevice->device, searchImageView, nullptr);
        pLogicalDevice->vkd.DestroyImage(pLogicalDevice->device, searchImage, nullptr);

        pLogicalDevice->vkd.DestroySampler(pLogicalDevice->device, sampler, nullptr);
    }

} // namespace vkBasalt
