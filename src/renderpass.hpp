#ifndef RENDERPASS_HPP_INCLUDED
#define RENDERPASS_HPP_INCLUDED

#include "logical_device.hpp"
#include "vulkan_include.hpp"

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    inline VkRenderPass createRenderPass(LogicalDevice* pLogicalDevice, VkFormat format)
    {

        const VkAttachmentDescription attachmentDescription{.flags          = 0,
                                                            .format         = format,
                                                            .samples        = VK_SAMPLE_COUNT_1_BIT,
                                                            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                                                            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                                            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                                            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
                                                            .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};

        const VkAttachmentReference attachmentReference{.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

        const VkSubpassDescription subpassDescription{.flags                   = 0,
                                                      .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                      .inputAttachmentCount    = 0,
                                                      .pInputAttachments       = nullptr,
                                                      .colorAttachmentCount    = 1,
                                                      .pColorAttachments       = std::addressof(attachmentReference),
                                                      .pResolveAttachments     = nullptr,
                                                      .pDepthStencilAttachment = nullptr,
                                                      .preserveAttachmentCount = 0,
                                                      .pPreserveAttachments    = nullptr};

        const VkSubpassDependency subpassDependency{.srcSubpass      = VK_SUBPASS_EXTERNAL,
                                                    .dstSubpass      = 0,
                                                    .srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                    .dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                    .srcAccessMask   = 0,
                                                    .dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                                    .dependencyFlags = 0};

        const VkRenderPassCreateInfo renderPassCreateInfo{.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                                                          .pNext           = nullptr,
                                                          .flags           = 0,
                                                          .attachmentCount = 1,
                                                          .pAttachments    = std::addressof(attachmentDescription),
                                                          .subpassCount    = 1,
                                                          .pSubpasses      = std::addressof(subpassDescription),
                                                          .dependencyCount = 1,
                                                          .pDependencies   = std::addressof(subpassDependency)};

        VkRenderPass renderPass{};
        const auto   result =
            pLogicalDevice->vkd.CreateRenderPass(pLogicalDevice->device, std::addressof(renderPassCreateInfo), nullptr, std::addressof(renderPass));
        AssertVulkan(result);

        return renderPass;
    }
}

#endif // RENDERPASS_HPP_INCLUDED
