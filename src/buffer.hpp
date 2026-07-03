#ifndef BUFFER_HPP_INCLUDED
#define BUFFER_HPP_INCLUDED

#include "logical_device.hpp"
#include "memory.hpp"
#include "vulkan_include.hpp"

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    inline void createBuffer(LogicalDevice*        pLogicalDevice,
                             VkDeviceSize          size,
                             VkBufferUsageFlags    usage,
                             VkMemoryPropertyFlags properties,
                             VkBuffer&             buffer,
                             VkDeviceMemory&       bufferMemory)
    {
        const VkBufferCreateInfo bufferInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = size, .usage = usage, .sharingMode = VK_SHARING_MODE_EXCLUSIVE};

        auto result = pLogicalDevice->vkd.CreateBuffer(pLogicalDevice->device, std::addressof(bufferInfo), nullptr, std::addressof(buffer));
        AssertVulkan(result);

        VkMemoryRequirements memRequirements;
        pLogicalDevice->vkd.GetBufferMemoryRequirements(pLogicalDevice->device, buffer, std::addressof(memRequirements));

        const VkMemoryAllocateInfo allocInfo = {.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                                .allocationSize  = memRequirements.size,
                                                .memoryTypeIndex = findMemoryTypeIndex(pLogicalDevice, memRequirements.memoryTypeBits, properties)};

        result = pLogicalDevice->vkd.AllocateMemory(pLogicalDevice->device, std::addressof(allocInfo), nullptr, std::addressof(bufferMemory));
        AssertVulkan(result);

        result = pLogicalDevice->vkd.BindBufferMemory(pLogicalDevice->device, buffer, bufferMemory, 0);
        AssertVulkan(result);
    }
}

#endif // BUFFER_HPP_INCLUDED
