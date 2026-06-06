#ifndef FAKE_SWAPCHAIN_HPP_INCLUDED
#define FAKE_SWAPCHAIN_HPP_INCLUDED

#include "logical_device.hpp"
#include "logical_swapchain.hpp"

#include <cstdint>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    std::vector<VkImage> createFakeSwapchainImages(LogicalDevice& logicalDevice, LogicalSwapchain& logicalSwapchain, uint32_t effectsCount);
}

#endif // FAKE_SWAPCHAIN_HPP_INCLUDED
