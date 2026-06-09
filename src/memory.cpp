#include "memory.hpp"
#include "logical_device.hpp"

#include <cstdint>
#include <logger.hpp>
#include <memory>
#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    uint32_t findMemoryTypeIndex(LogicalDevice* pLogicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
        pLogicalDevice->vki.GetPhysicalDeviceMemoryProperties(pLogicalDevice->physicalDevice, std::addressof(physicalDeviceMemoryProperties));
        for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i)
        {
            if (((typeFilter & (1U << i)) != 0U) && (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        Logger::err("Found no correct memory type");
        return 0x70AD;
    }
} // namespace vkBasalt
