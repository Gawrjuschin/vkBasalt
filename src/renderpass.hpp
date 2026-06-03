#ifndef RENDERPASS_HPP_INCLUDED
#define RENDERPASS_HPP_INCLUDED

#include "logical_device.hpp"
#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    VkRenderPass createRenderPass(LogicalDevice* pLogicalDevice, VkFormat format);
}

#endif // RENDERPASS_HPP_INCLUDED
