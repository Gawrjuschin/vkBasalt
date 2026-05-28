#ifndef BUFFER_HPP_INCLUDED
#define BUFFER_HPP_INCLUDED

#include "logical_device.hpp"

namespace vkBasalt
{
    void createBuffer(LogicalDevice*        pLogicalDevice,
                      VkDeviceSize          size,
                      VkBufferUsageFlags    usage,
                      VkMemoryPropertyFlags properties,
                      VkBuffer&             buffer,
                      VkDeviceMemory&       bufferMemory);
}

#endif // BUFFER_HPP_INCLUDED
