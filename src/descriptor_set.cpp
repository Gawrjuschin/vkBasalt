#include "descriptor_set.hpp"
#include "logical_device.hpp"
#include "logger.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <ranges>
#include <vector>

#include <vulkan_include.hpp>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{

    VkDescriptorPool createDescriptorPool(LogicalDevice* pLogicalDevice, std::span<const VkDescriptorPoolSize> poolSizes)
    {

        const auto setCount{
            std::ranges::fold_left(poolSizes | std::views::transform(&VkDescriptorPoolSize::descriptorCount), 0U, std::plus<uint32_t>{})};

        const VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                                                  .pNext         = nullptr,
                                                                  .flags         = 0,
                                                                  .maxSets       = setCount,
                                                                  .poolSizeCount = static_cast<uint32_t>(std::size(poolSizes)),
                                                                  .pPoolSizes    = std::data(poolSizes)};

        VkDescriptorPool descriptorPool{};
        const auto       result = pLogicalDevice->vkd.CreateDescriptorPool(
            pLogicalDevice->device, std::addressof(descriptorPoolCreateInfo), nullptr, std::addressof(descriptorPool));
        AssertVulkan(result);
        return descriptorPool;
    }

    VkDescriptorSetLayout createUniformBufferDescriptorSetLayout(LogicalDevice* pLogicalDevice)
    {

        const VkDescriptorSetLayoutBinding descriptorSetLayoutBinding{.binding            = 0,
                                                                      .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                                      .descriptorCount    = 1,
                                                                      .stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
                                                                      .pImmutableSamplers = nullptr};

        const VkDescriptorSetLayoutCreateInfo descriptorSetCreateInfo{.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                                      .pNext        = nullptr,
                                                                      .flags        = 0,
                                                                      .bindingCount = 1,
                                                                      .pBindings    = std::addressof(descriptorSetLayoutBinding)};

        VkDescriptorSetLayout descriptorSetLayout{};
        const auto            result = pLogicalDevice->vkd.CreateDescriptorSetLayout(
            pLogicalDevice->device, std::addressof(descriptorSetCreateInfo), nullptr, std::addressof(descriptorSetLayout));
        AssertVulkan(result);

        return descriptorSetLayout;
    }

    VkDescriptorSet writeBufferDescriptorSet(LogicalDevice*        pLogicalDevice,
                                             VkDescriptorPool      descriptorPool,
                                             VkDescriptorSetLayout descriptorSetLayout,
                                             VkBuffer              buffer)
    {
        VkDescriptorSet descriptorSet{};

        const VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                                                    .pNext              = nullptr,
                                                                    .descriptorPool     = descriptorPool,
                                                                    .descriptorSetCount = 1,
                                                                    .pSetLayouts        = std::addressof(descriptorSetLayout)};

        const auto result = pLogicalDevice->vkd.AllocateDescriptorSets(
            pLogicalDevice->device, std::addressof(descriptorSetAllocateInfo), std::addressof(descriptorSet));
        AssertVulkan(result);

        const VkDescriptorBufferInfo bufferInfo{.buffer = buffer, .offset = 0, .range = VK_WHOLE_SIZE};

        const VkWriteDescriptorSet writeDescriptorSet{.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                                      .pNext            = nullptr,
                                                      .dstSet           = descriptorSet,
                                                      .dstBinding       = 0,
                                                      .dstArrayElement  = 0,
                                                      .descriptorCount  = 1,
                                                      .descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                      .pImageInfo       = nullptr,
                                                      .pBufferInfo      = std::addressof(bufferInfo),
                                                      .pTexelBufferView = nullptr};

        Logger::debug("before writing buffer descriptor Sets");
        pLogicalDevice->vkd.UpdateDescriptorSets(pLogicalDevice->device, 1, std::addressof(writeDescriptorSet), 0, nullptr);

        return descriptorSet;
    }

    VkDescriptorSetLayout createImageSamplerDescriptorSetLayout(LogicalDevice* pLogicalDevice, uint32_t count)
    {
        VkDescriptorSetLayout descriptorSetLayout{};

        std::vector<VkDescriptorSetLayoutBinding> bindigs(count);
        for (auto [idx, bindig] : bindigs | std::views::enumerate)
        {
            bindig = {.binding            = static_cast<uint32_t>(idx),
                      .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                      .descriptorCount    = 1,
                      .stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
                      .pImmutableSamplers = nullptr};
        }

        const VkDescriptorSetLayoutCreateInfo descriptorSetCreateInfo{.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                                      .pNext        = nullptr,
                                                                      .flags        = 0,
                                                                      .bindingCount = static_cast<uint32_t>(std::size(bindigs)),
                                                                      .pBindings    = std::data(bindigs)};

        const auto result = pLogicalDevice->vkd.CreateDescriptorSetLayout(
            pLogicalDevice->device, std::addressof(descriptorSetCreateInfo), nullptr, std::addressof(descriptorSetLayout));
        AssertVulkan(result);
        return descriptorSetLayout;
    }

    std::vector<VkDescriptorSet> allocateAndWriteImageSamplerDescriptorSets(LogicalDevice*                        pLogicalDevice,
                                                                            VkDescriptorPool                      descriptorPool,
                                                                            VkDescriptorSetLayout                 descriptorSetLayout,
                                                                            std::vector<VkSampler>                samplers,
                                                                            std::vector<std::vector<VkImageView>> imageViewsVectors)
    {
        std::vector<VkDescriptorSet> descriptorSets(std::size(imageViewsVectors.front()));

        std::vector<VkDescriptorSetLayout> layouts(std::size(descriptorSets), descriptorSetLayout);
        const VkDescriptorSetAllocateInfo  descriptorSetAllocateInfo{.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                                                     .pNext              = nullptr,
                                                                     .descriptorPool     = descriptorPool,
                                                                     .descriptorSetCount = static_cast<uint32_t>(std::size(descriptorSets)),
                                                                     .pSetLayouts        = std::data(layouts)};

        Logger::debug("before allocating descriptor Sets");
        const auto result =
            pLogicalDevice->vkd.AllocateDescriptorSets(pLogicalDevice->device, std::addressof(descriptorSetAllocateInfo), std::data(descriptorSets));
        AssertVulkan(result);

        std::vector<VkDescriptorImageInfo> imageInfos(
            std::size(imageViewsVectors),
            {.sampler = VK_NULL_HANDLE, .imageView = VK_NULL_HANDLE, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});

        std::vector<VkWriteDescriptorSet> writeDescriptorSets(std::size(imageViewsVectors),
                                                              {.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                                               .pNext            = nullptr,
                                                               .dstSet           = VK_NULL_HANDLE,
                                                               .dstBinding       = 0,
                                                               .dstArrayElement  = 0,
                                                               .descriptorCount  = 1,
                                                               .descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                               .pImageInfo       = nullptr,
                                                               .pBufferInfo      = nullptr,
                                                               .pTexelBufferView = nullptr});

        for (std::size_t i{0}; i < std::size(descriptorSets); ++i)
        {
            for (std::size_t j{0}; j < std::size(imageViewsVectors); ++j)
            {
                imageInfos[j].sampler   = samplers[j];
                imageInfos[j].imageView = imageViewsVectors[j][i]; // TODO: maybe error!

                writeDescriptorSets[j].dstBinding = j;
                writeDescriptorSets[j].pImageInfo = std::addressof(imageInfos[j]);
                writeDescriptorSets[j].dstSet     = descriptorSets[i];
            }
            Logger::debug("before writing descriptor Sets");
            pLogicalDevice->vkd.UpdateDescriptorSets(
                pLogicalDevice->device, std::size(writeDescriptorSets), std::data(writeDescriptorSets), 0, nullptr);
        }
        return descriptorSets;
    }
} // namespace vkBasalt
