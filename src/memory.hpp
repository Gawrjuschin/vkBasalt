#ifndef MEMORY_HPP_INCLUDED
#define MEMORY_HPP_INCLUDED

#include "logical_device.hpp"
#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    uint32_t findMemoryTypeIndex(LogicalDevice* pLogicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
}

#endif // MEMORY_HPP_INCLUDED
