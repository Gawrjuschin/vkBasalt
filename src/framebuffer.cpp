#include "framebuffer.hpp"
#include "logical_device.hpp"
#include "vulkan_include.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <cstdint>

namespace vkBasalt
{
    std::vector<VkFramebuffer>
    createFramebuffers(LogicalDevice* pLogicalDevice, VkRenderPass renderPass, VkExtent2D& extent, std::vector<std::vector<VkImageView>> imageViews)
    {
        std::vector<VkFramebuffer> framebuffers(std::size(imageViews.front()));

        std::vector<VkImageView> perFrameImageViews;
        perFrameImageViews.resize(std::size(imageViews));

        for (uint32_t i = 0; i < std::size(imageViews.front()); ++i)
        {
            perFrameImageViews.clear();
            for (auto& view : imageViews)
            {
                perFrameImageViews.push_back(view[i]);
            }

            VkFramebufferCreateInfo framebufferCreateInfo{.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                                                          .pNext           = nullptr,
                                                          .flags           = 0,
                                                          .renderPass      = renderPass,
                                                          .attachmentCount = static_cast<uint32_t>(std::size(perFrameImageViews)),
                                                          .pAttachments    = std::data(perFrameImageViews),
                                                          .width           = extent.width,
                                                          .height          = extent.height,
                                                          .layers          = 1};

            const auto result = pLogicalDevice->vkd.CreateFramebuffer(
                pLogicalDevice->device, std::addressof(framebufferCreateInfo), nullptr, std::addressof(framebuffers[i]));
            AssertVulkan(result);
        }
        return framebuffers;
    }
} // namespace vkBasalt
