#include "shader.hpp"
#include "logical_device.hpp"
#include "vulkan_include.hpp"
#include <memory>
#include <span>
#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    void createShaderModule(LogicalDevice* pLogicalDevice, std::span<const uint32_t> code, VkShaderModule* shaderModule)
    {
        const VkShaderModuleCreateInfo shaderCreateInfo{
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext    = nullptr,
            .flags    = 0,
            .codeSize = code.size_bytes(),
            .pCode    = std::data(code),
        };

        const auto result = pLogicalDevice->vkd.CreateShaderModule(pLogicalDevice->device, std::addressof(shaderCreateInfo), nullptr, shaderModule);
        AssertVulkan(result);
    }
} // namespace vkBasalt
