#ifndef MEMORY_HPP_INCLUDED
#define MEMORY_HPP_INCLUDED

#include "logical_device.hpp"

#include <cstdint>
#include <ranges>
#include <span>

#include <vulkan/vulkan_core.h>

#include <logger.hpp>

namespace vkBasalt
{
    inline uint32_t findMemoryTypeIndex(LogicalDevice* pLogicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
        pLogicalDevice->vki.GetPhysicalDeviceMemoryProperties(pLogicalDevice->physicalDevice, std::addressof(physicalDeviceMemoryProperties));
        for (auto [typeIndex, memoryType] :
             std::span{physicalDeviceMemoryProperties.memoryTypes, physicalDeviceMemoryProperties.memoryTypeCount} | std::views::enumerate)
        {
            if (((typeFilter & (1U << typeIndex)) != 0U) && (memoryType.propertyFlags & properties) == properties)
            {
                return typeIndex;
            }
        }

        Logger::err("Found no correct memory type");
        return 0x70AD; // TODO: better solution
    }
}

#endif // MEMORY_HPP_INCLUDED
