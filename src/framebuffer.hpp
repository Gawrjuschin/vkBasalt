#ifndef FRAMEBUFFER_HPP_INCLUDED
#define FRAMEBUFFER_HPP_INCLUDED

#include "logical_device.hpp"

#include <vector>

namespace vkBasalt
{
    std::vector<VkFramebuffer>
    createFramebuffers(LogicalDevice* pLogicalDevice, VkRenderPass renderPass, VkExtent2D& extent, std::vector<std::vector<VkImageView>> imageViews);
}

#endif // FRAMEBUFFER_HPP_INCLUDED
