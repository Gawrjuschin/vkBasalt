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

        VkDescriptorPool descriptorPool{};

        const auto setCount{
            std::ranges::fold_left(poolSizes | std::views::transform(&VkDescriptorPoolSize::descriptorCount), 0U, std::plus<uint32_t>{})};

        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
        descriptorPoolCreateInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolCreateInfo.pNext         = nullptr;
        descriptorPoolCreateInfo.flags         = 0;
        descriptorPoolCreateInfo.maxSets       = setCount;
        descriptorPoolCreateInfo.poolSizeCount = std::size(poolSizes);
        descriptorPoolCreateInfo.pPoolSizes    = std::data(poolSizes);

        const auto result = pLogicalDevice->vkd.CreateDescriptorPool(pLogicalDevice->device, &descriptorPoolCreateInfo, nullptr, &descriptorPool);
        AssertVulkan(result);
        return descriptorPool;
    }

    VkDescriptorSetLayout createUniformBufferDescriptorSetLayout(LogicalDevice* pLogicalDevice)
    {
        VkDescriptorSetLayout descriptorSetLayout{};

        VkDescriptorSetLayoutBinding descriptorSetLayoutBinding;
        descriptorSetLayoutBinding.binding            = 0;
        descriptorSetLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorSetLayoutBinding.descriptorCount    = 1;
        descriptorSetLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo descriptorSetCreateInfo;
        descriptorSetCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetCreateInfo.pNext        = nullptr;
        descriptorSetCreateInfo.flags        = 0;
        descriptorSetCreateInfo.bindingCount = 1;
        descriptorSetCreateInfo.pBindings    = &descriptorSetLayoutBinding;

        const auto result =
            pLogicalDevice->vkd.CreateDescriptorSetLayout(pLogicalDevice->device, &descriptorSetCreateInfo, nullptr, &descriptorSetLayout);
        AssertVulkan(result);

        return descriptorSetLayout;
    }

    VkDescriptorSet writeBufferDescriptorSet(LogicalDevice*        pLogicalDevice,
                                             VkDescriptorPool      descriptorPool,
                                             VkDescriptorSetLayout descriptorSetLayout,
                                             VkBuffer              buffer)
    {
        VkDescriptorSet descriptorSet{};

        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
        descriptorSetAllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.pNext              = nullptr;
        descriptorSetAllocateInfo.descriptorPool     = descriptorPool;
        descriptorSetAllocateInfo.descriptorSetCount = 1;
        descriptorSetAllocateInfo.pSetLayouts        = &descriptorSetLayout;

        const auto result = pLogicalDevice->vkd.AllocateDescriptorSets(pLogicalDevice->device, &descriptorSetAllocateInfo, &descriptorSet);
        AssertVulkan(result);

        VkDescriptorBufferInfo bufferInfo;
        bufferInfo.buffer = buffer;
        bufferInfo.offset = 0;
        bufferInfo.range  = VK_WHOLE_SIZE;

        VkWriteDescriptorSet writeDescriptorSet = {};

        writeDescriptorSet.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.pNext            = nullptr;
        writeDescriptorSet.dstSet           = descriptorSet;
        writeDescriptorSet.dstBinding       = 0;
        writeDescriptorSet.dstArrayElement  = 0;
        writeDescriptorSet.descriptorCount  = 1;
        writeDescriptorSet.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSet.pImageInfo       = nullptr;
        writeDescriptorSet.pBufferInfo      = &bufferInfo;
        writeDescriptorSet.pTexelBufferView = nullptr;

        Logger::debug("before writing buffer descriptor Sets");
        pLogicalDevice->vkd.UpdateDescriptorSets(pLogicalDevice->device, 1, &writeDescriptorSet, 0, nullptr);

        return descriptorSet;
    }

    VkDescriptorSetLayout createImageSamplerDescriptorSetLayout(LogicalDevice* pLogicalDevice, uint32_t count)
    {
        VkDescriptorSetLayout descriptorSetLayout{};

        std::vector<VkDescriptorSetLayoutBinding> bindigs(count);
        for (uint32_t i = 0; i < count; i++)
        {
            VkDescriptorSetLayoutBinding descriptorSetLayoutBinding;
            descriptorSetLayoutBinding.binding            = i;
            descriptorSetLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorSetLayoutBinding.descriptorCount    = 1;
            descriptorSetLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
            descriptorSetLayoutBinding.pImmutableSamplers = nullptr;
            bindigs[i]                                    = descriptorSetLayoutBinding;
        }

        VkDescriptorSetLayoutCreateInfo descriptorSetCreateInfo;
        descriptorSetCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetCreateInfo.pNext        = nullptr;
        descriptorSetCreateInfo.flags        = 0;
        descriptorSetCreateInfo.bindingCount = count;
        descriptorSetCreateInfo.pBindings    = bindigs.data();

        const auto result =
            pLogicalDevice->vkd.CreateDescriptorSetLayout(pLogicalDevice->device, &descriptorSetCreateInfo, nullptr, &descriptorSetLayout);
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

        std::vector<VkDescriptorSetLayout> layouts(descriptorSets.size(), descriptorSetLayout);
        VkDescriptorSetAllocateInfo        descriptorSetAllocateInfo;
        descriptorSetAllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.pNext              = nullptr;
        descriptorSetAllocateInfo.descriptorPool     = descriptorPool;
        descriptorSetAllocateInfo.descriptorSetCount = descriptorSets.size();
        descriptorSetAllocateInfo.pSetLayouts        = layouts.data();

        Logger::debug("before allocating descriptor Sets");
        const auto result = pLogicalDevice->vkd.AllocateDescriptorSets(pLogicalDevice->device, &descriptorSetAllocateInfo, descriptorSets.data());
        AssertVulkan(result);

        ;

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
