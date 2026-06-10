#include "memory.hpp"
#include "logical_device.hpp"

#include <cstdint>
#include <logger.hpp>
#include <memory>
#include <ranges>
#include <span>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    uint32_t findMemoryTypeIndex(LogicalDevice* pLogicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
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
} // namespace vkBasalt
