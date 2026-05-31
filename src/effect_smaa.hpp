#ifndef EFFECT_SMAA_HPP_INCLUDED
#define EFFECT_SMAA_HPP_INCLUDED

#include "effect.hpp"
#include "config.hpp"
#include "logical_device.hpp"

#include <vector>

namespace vkBasalt
{
    class SmaaEffect final : public Effect
    {
    public:
        SmaaEffect(LogicalDevice*       pLogicalDevice,
                   VkFormat             format,
                   VkExtent2D           imageExtent,
                   std::vector<VkImage> inputImages,
                   std::vector<VkImage> outputImages,
                   Config*              pConfig);
        void applyEffect(uint32_t imageIndex, VkCommandBuffer commandBuffer) override;
        ~SmaaEffect() override;

    private:
        LogicalDevice*               pLogicalDevice;
        std::vector<VkImage>         inputImages;
        std::vector<VkImage>         edgeImages;
        std::vector<VkImage>         blendImages;
        std::vector<VkImage>         outputImages;
        std::vector<VkImageView>     inputImageViews;
        std::vector<VkImageView>     edgeImageViews;
        std::vector<VkImageView>     blendImageViews;
        std::vector<VkImageView>     outputImageViews;
        std::vector<VkDescriptorSet> imageDescriptorSets;
        std::vector<VkFramebuffer>   edgeFramebuffers;
        std::vector<VkFramebuffer>   blendFramebuffers;
        std::vector<VkFramebuffer>   neignborFramebuffers;
        VkImage                      areaImage;
        VkImage                      searchImage;
        VkImageView                  areaImageView;
        VkImageView                  searchImageView;
        VkDescriptorSetLayout        imageSamplerDescriptorSetLayout;
        VkDescriptorPool             descriptorPool;
        VkShaderModule               edgeVertexModule;
        VkShaderModule               edgeFragmentModule;
        VkShaderModule               blendVertexModule;
        VkShaderModule               blendFragmentModule;
        VkShaderModule               neighborVertexModule;
        VkShaderModule               neignborFragmentModule;
        VkRenderPass                 renderPass;
        VkRenderPass                 unormRenderPass;
        VkPipelineLayout             pipelineLayout;
        VkPipeline                   edgePipeline;
        VkPipeline                   blendPipeline;
        VkPipeline                   neighborPipeline;
        VkExtent2D                   imageExtent;
        VkFormat                     format;
        VkDeviceMemory               imageMemory;
        VkDeviceMemory               areaMemory;
        VkDeviceMemory               searchMemory;
        VkSampler                    sampler;

        Config* pConfig;
    };
} // namespace vkBasalt

#endif // EFFECT_SMAA_HPP_INCLUDED
