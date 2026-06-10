#ifndef EFFECT_SIMPLE_HPP_INCLUDED
#define EFFECT_SIMPLE_HPP_INCLUDED

#include "effect.hpp"
#include "config.hpp"
#include "logical_device.hpp"

#include <cstdint>
#include <vector>
#include <span>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    class SimpleEffect : public Effect
    {
    public:
        SimpleEffect();
        SimpleEffect(const SimpleEffect&)            = delete;
        SimpleEffect& operator=(const SimpleEffect&) = delete;
        SimpleEffect(SimpleEffect&&)                 = delete;
        SimpleEffect& operator=(SimpleEffect&&)      = delete;
        ~SimpleEffect() override;

        void applyEffect(uint32_t imageIndex, VkCommandBuffer commandBuffer) override;

    protected:
        LogicalDevice*               pLogicalDevice{};
        std::vector<VkImage>         inputImages;
        std::vector<VkImage>         outputImages;
        std::vector<VkImageView>     inputImageViews;
        std::vector<VkImageView>     outputImageViews;
        std::vector<VkDescriptorSet> imageDescriptorSets;
        std::vector<VkFramebuffer>   framebuffers;
        VkDescriptorSetLayout        imageSamplerDescriptorSetLayout{};
        VkDescriptorPool             descriptorPool{};
        VkShaderModule               vertexModule{};
        VkShaderModule               fragmentModule{};
        VkRenderPass                 renderPass{};
        VkPipelineLayout             pipelineLayout{};
        VkPipeline                   graphicsPipeline{};
        VkExtent2D                   imageExtent{};
        VkFormat                     format{};
        VkSampler                    sampler{};
        Config*                      pConfig{};
        std::span<const uint32_t>    vertexCode;
        std::span<const uint32_t>    fragmentCode;
        const VkSpecializationInfo*  pVertexSpecInfo{};
        const VkSpecializationInfo*  pFragmentSpecInfo{};

        // subclasses can put DescriptorSets in here, but the first one will be the input image descriptorSet
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

        void init(LogicalDevice*           pLogicalDevice,
                  VkFormat                 format,
                  VkExtent2D               imageExtent,
                  std::span<const VkImage> inputImages,
                  std::span<const VkImage> outputImages,
                  Config*                  pConfig);
    };
} // namespace vkBasalt

#endif // EFFECT_SIMPLE_HPP_INCLUDED
