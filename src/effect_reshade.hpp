#ifndef EFFECT_RESHADE_HPP_INCLUDED
#define EFFECT_RESHADE_HPP_INCLUDED

#include "effect.hpp"
#include "config.hpp"
#include "effect_module.hpp"
#include "reshade_uniforms.hpp"
#include "logical_device.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <memory>
#include <span>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    class ReshadeEffect final : public Effect
    {
    public:
        ReshadeEffect(LogicalDevice*           pLogicalDevice,
                      VkFormat                 format,
                      VkExtent2D               imageExtent,
                      std::span<const VkImage> inputImages,
                      std::span<const VkImage> outputImages,
                      Config*                  pConfig,
                      std::string_view         effectName);
        ReshadeEffect(const ReshadeEffect&)            = delete;
        ReshadeEffect& operator=(const ReshadeEffect&) = delete;
        ReshadeEffect(ReshadeEffect&&)                 = delete;
        ReshadeEffect& operator=(ReshadeEffect&&)      = delete;
        ~ReshadeEffect() override;

        void applyEffect(uint32_t imageIndex, VkCommandBuffer commandBuffer) override;
        void updateEffect() override;
        void useDepthImage(VkImageView depthImageView) override;

    private:
        LogicalDevice*           pLogicalDevice;
        std::vector<VkImage>     inputImages;
        std::vector<VkImage>     outputImages;
        std::vector<VkImageView> inputImageViewsSRGB;
        std::vector<VkImageView> inputImageViewsUNORM;
        std::vector<VkImageView> outputImageViewsSRGB;
        std::vector<VkImageView> outputImageViewsUNORM;

        std::unordered_map<std::string, std::vector<VkImage>>     textureImages;
        std::unordered_map<std::string, std::vector<VkImageView>> textureImageViewsUNORM;
        std::unordered_map<std::string, std::vector<VkImageView>> textureImageViewsSRGB;
        std::unordered_map<std::string, std::vector<VkImageView>> renderImageViewsSRGB;
        std::unordered_map<std::string, std::vector<VkImageView>> renderImageViewsUNORM;

        std::unordered_map<std::string, VkFormat>   textureFormatsUNORM;
        std::unordered_map<std::string, VkFormat>   textureFormatsSRGB;
        std::unordered_map<std::string, uint32_t>   textureMipLevels;
        std::unordered_map<std::string, VkExtent3D> textureExtents;

        std::vector<VkDescriptorSet> inputDescriptorSets;
        std::vector<VkDescriptorSet> outputDescriptorSets;
        std::vector<VkDescriptorSet> backBufferDescriptorSets;

        std::vector<std::vector<VkFramebuffer>> framebuffers;

        VkDescriptorSetLayout                 uniformDescriptorSetLayout;
        VkDescriptorSetLayout                 imageSamplerDescriptorSetLayout;
        VkShaderModule                        shaderModule{};
        VkDescriptorPool                      descriptorPool;
        std::vector<VkRenderPass>             renderPasses;
        std::vector<std::vector<std::string>> renderTargets;
        std::vector<VkRenderPassBeginInfo>    renderPassBeginInfos;
        VkPipelineLayout                      pipelineLayout;
        std::vector<VkPipeline>               graphicsPipelines;
        std::vector<bool>                     switchSamplers;
        VkExtent2D                            imageExtent;
        std::vector<VkSampler>                samplers;
        Config*                               pConfig;
        std::string                           effectName;
        reshadefx::module                     module;
        std::vector<VkDeviceMemory>           textureMemory;

        VkFormat    inputOutputFormatUNORM;
        VkFormat    inputOutputFormatSRGB;
        VkFormat    stencilFormat;
        VkImage     stencilImage;
        VkImageView stencilImageView;
        // how often the shader writes to the reshade back buffer
        // we need to flip the "backbuffer" after each write if there is a next one
        int                      outputWrites{};
        std::vector<VkImage>     backBufferImages;
        std::vector<VkImageView> backBufferImageViewsUNORM;
        std::vector<VkImageView> backBufferImageViewsSRGB;
        VkBuffer                 stagingBuffer{};
        VkDeviceMemory           stagingBufferMemory{};
        uint32_t                 bufferSize{};
        VkDescriptorSet          bufferDescriptorSet{};

        std::vector<std::unique_ptr<ReshadeUniform>> uniforms;

        void createReshadeModule();
    };
} // namespace vkBasalt

#endif // EFFECT_RESHADE_HPP_INCLUDED
