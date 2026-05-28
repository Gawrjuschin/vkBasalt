#ifndef FAKE_SWAPCHAIN_HPP_INCLUDED
#define FAKE_SWAPCHAIN_HPP_INCLUDED

#include "logical_device.hpp"

#include <vector>

namespace vkBasalt
{
    std::vector<VkImage> createFakeSwapchainImages(LogicalDevice*           pLogicalDevice,
                                                   VkSwapchainCreateInfoKHR swapchainCreateInfo,
                                                   uint32_t                 count,
                                                   VkDeviceMemory&          deviceMemory);
}

#endif // FAKE_SWAPCHAIN_HPP_INCLUDED
