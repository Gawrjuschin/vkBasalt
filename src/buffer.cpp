#include "buffer.hpp"
#include "memory.hpp"

#include <vulkan_include.hpp>

namespace vkBasalt
{
    void createBuffer(LogicalDevice*        pLogicalDevice,
                      VkDeviceSize          size,
                      VkBufferUsageFlags    usage,
                      VkMemoryPropertyFlags properties,
                      VkBuffer&             buffer,
                      VkDeviceMemory&       bufferMemory)
    {
        VkBufferCreateInfo bufferInfo = {};

        bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size        = size;
        bufferInfo.usage       = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = pLogicalDevice->vkd.CreateBuffer(pLogicalDevice->device, &bufferInfo, nullptr, &buffer);
        AssertVulkan(result);

        VkMemoryRequirements memRequirements;
        pLogicalDevice->vkd.GetBufferMemoryRequirements(pLogicalDevice->device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};

        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryTypeIndex(pLogicalDevice, memRequirements.memoryTypeBits, properties);

        result = pLogicalDevice->vkd.AllocateMemory(pLogicalDevice->device, &allocInfo, nullptr, &bufferMemory);
        AssertVulkan(result);

        result = pLogicalDevice->vkd.BindBufferMemory(pLogicalDevice->device, buffer, bufferMemory, 0);
        AssertVulkan(result);
    }

} // namespace vkBasalt
