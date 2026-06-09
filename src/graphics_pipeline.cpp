#include "graphics_pipeline.hpp"
#include "logical_device.hpp"
#include "vulkan_include.hpp"

#include <array>
#include <iterator>
#include <memory>
#include <string>
#include <span>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    VkPipelineLayout createGraphicsPipelineLayout(LogicalDevice* pLogicalDevice, std::span<VkDescriptorSetLayout> descriptorSetLayouts)
    {
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
        pipelineLayoutCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.pNext                  = nullptr;
        pipelineLayoutCreateInfo.flags                  = 0;
        pipelineLayoutCreateInfo.setLayoutCount         = descriptorSetLayouts.size();
        pipelineLayoutCreateInfo.pSetLayouts            = descriptorSetLayouts.data();
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.pPushConstantRanges    = nullptr;

        VkPipelineLayout pipelineLayout{};
        const auto       result = pLogicalDevice->vkd.CreatePipelineLayout(
            pLogicalDevice->device, std::addressof(pipelineLayoutCreateInfo), nullptr, std::addressof(pipelineLayout));
        AssertVulkan(result);
        return pipelineLayout;
    }

    VkPipeline createGraphicsPipeline(LogicalDevice*        pLogicalDevice,
                                      VkShaderModule        vertexModule,
                                      VkSpecializationInfo* vertexSpecializationInfo,
                                      std::string           vertexEntryPoint,
                                      VkShaderModule        fragmentModule,
                                      VkSpecializationInfo* fragmentSpecializationInfo,
                                      std::string           fragmentEntryPoint,
                                      VkExtent2D            extent,
                                      VkRenderPass          renderPass,
                                      VkPipelineLayout      pipelineLayout,
                                      bool                  flip)
    {
        VkResult result{};

        VkPipeline pipeline{};

        const VkPipelineShaderStageCreateInfo shaderStageCreateInfoVert{.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                                        .pNext               = nullptr,
                                                                        .flags               = 0,
                                                                        .stage               = VK_SHADER_STAGE_VERTEX_BIT,
                                                                        .module              = vertexModule,
                                                                        .pName               = vertexEntryPoint.c_str(),
                                                                        .pSpecializationInfo = vertexSpecializationInfo};

        const VkPipelineShaderStageCreateInfo shaderStageCreateInfoFrag{.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                                        .pNext               = nullptr,
                                                                        .flags               = 0,
                                                                        .stage               = VK_SHADER_STAGE_FRAGMENT_BIT,
                                                                        .module              = fragmentModule,
                                                                        .pName               = fragmentEntryPoint.c_str(),
                                                                        .pSpecializationInfo = fragmentSpecializationInfo};

        const std::array shaderStages{shaderStageCreateInfoVert, shaderStageCreateInfoFrag};

        VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                                                                   .pNext = nullptr,
                                                                   .flags = 0,
                                                                   .vertexBindingDescriptionCount   = 0,
                                                                   .pVertexBindingDescriptions      = nullptr,
                                                                   .vertexAttributeDescriptionCount = 0,
                                                                   .pVertexAttributeDescriptions    = nullptr};

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                                                                       .pNext    = nullptr,
                                                                       .flags    = 0,
                                                                       .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                                                                       .primitiveRestartEnable = VK_FALSE};

        VkViewport viewport{.x        = 0.0F,
                            .y        = flip ? static_cast<float>(extent.height) : 0.0F,
                            .width    = static_cast<float>(extent.width),
                            .height   = flip ? -static_cast<float>(extent.height) : static_cast<float>(extent.height),
                            .minDepth = 0.0F,
                            .maxDepth = 1.0F};

        VkRect2D scissor{.offset = {.x = 0, .y = 0}, .extent = {.width = extent.width, .height = extent.height}};

        VkPipelineViewportStateCreateInfo viewportStateCreateInfo{.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                                                                  .pNext         = nullptr,
                                                                  .flags         = 0,
                                                                  .viewportCount = 1,
                                                                  .pViewports    = std::addressof(viewport),
                                                                  .scissorCount  = 1,
                                                                  .pScissors     = std::addressof(scissor)};

        VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo{.sType            = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                                                                       .pNext            = nullptr,
                                                                       .flags            = 0,
                                                                       .depthClampEnable = VK_FALSE,
                                                                       .rasterizerDiscardEnable = VK_FALSE,
                                                                       .polygonMode             = VK_POLYGON_MODE_FILL,
                                                                       .cullMode                = VK_CULL_MODE_NONE,
                                                                       .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                                                                       .depthBiasEnable         = VK_FALSE,
                                                                       .depthBiasConstantFactor = 0.0F,
                                                                       .depthBiasClamp          = 0.0F,
                                                                       .depthBiasSlopeFactor    = 0.0F,
                                                                       .lineWidth               = 1.0F};

        VkPipelineMultisampleStateCreateInfo multisampleCreateInfo{.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                                                                   .pNext                 = nullptr,
                                                                   .flags                 = 0,
                                                                   .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
                                                                   .sampleShadingEnable   = VK_FALSE,
                                                                   .minSampleShading      = 1.0F,
                                                                   .pSampleMask           = nullptr,
                                                                   .alphaToCoverageEnable = VK_FALSE,
                                                                   .alphaToOneEnable      = VK_FALSE};

        // do not use blending, we only have one image and that should not be modified
        VkPipelineColorBlendAttachmentState colorBlendAttachment{.blendEnable         = VK_FALSE,
                                                                 .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                                                                 .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                                                                 .colorBlendOp        = VK_BLEND_OP_ADD,
                                                                 .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                                                                 .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                                                                 .alphaBlendOp        = VK_BLEND_OP_ADD,
                                                                 .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                                                                   | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

        VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo{.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                                                                 .pNext           = nullptr,
                                                                 .flags           = 0,
                                                                 .logicOpEnable   = VK_FALSE,
                                                                 .logicOp         = VK_LOGIC_OP_NO_OP,
                                                                 .attachmentCount = 1,
                                                                 .pAttachments    = std::addressof(colorBlendAttachment),
                                                                 .blendConstants  = {0.0F, 0.0F, 0.0F, 0.0F}};

        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                                                                .pNext             = nullptr,
                                                                .flags             = 0,
                                                                .dynamicStateCount = 0,
                                                                .pDynamicStates    = nullptr};

        VkGraphicsPipelineCreateInfo pipelineCreateInfo{.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                                                        .pNext               = nullptr,
                                                        .flags               = 0,
                                                        .stageCount          = std::size(shaderStages),
                                                        .pStages             = std::data(shaderStages),
                                                        .pVertexInputState   = std::addressof(vertexInputCreateInfo),
                                                        .pInputAssemblyState = std::addressof(inputAssemblyCreateInfo),
                                                        .pTessellationState  = nullptr,
                                                        .pViewportState      = std::addressof(viewportStateCreateInfo),
                                                        .pRasterizationState = std::addressof(rasterizationCreateInfo),
                                                        .pMultisampleState   = std::addressof(multisampleCreateInfo),
                                                        .pDepthStencilState  = nullptr,
                                                        .pColorBlendState    = std::addressof(colorBlendCreateInfo),
                                                        .pDynamicState       = std::addressof(dynamicStateCreateInfo),
                                                        .layout              = pipelineLayout,
                                                        .renderPass          = renderPass,
                                                        .subpass             = 0,
                                                        .basePipelineHandle  = VK_NULL_HANDLE,
                                                        .basePipelineIndex   = -1};

        result = pLogicalDevice->vkd.CreateGraphicsPipelines(
            pLogicalDevice->device, VK_NULL_HANDLE, 1, std::addressof(pipelineCreateInfo), nullptr, std::addressof(pipeline));
        AssertVulkan(result);

        return pipeline;
    }
} // namespace vkBasalt
