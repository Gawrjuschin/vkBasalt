#include "shader.hpp"

namespace vkBasalt
{
    void createShaderModule(LogicalDevice* pLogicalDevice, std::span<const uint32_t> code, VkShaderModule* shaderModule)
    {
        VkShaderModuleCreateInfo shaderCreateInfo;

        shaderCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderCreateInfo.pNext    = nullptr;
        shaderCreateInfo.flags    = 0;
        shaderCreateInfo.codeSize = code.size_bytes();
        shaderCreateInfo.pCode    = code.data();

        VkResult result = pLogicalDevice->vkd.CreateShaderModule(pLogicalDevice->device, &shaderCreateInfo, nullptr, shaderModule);
        ASSERT_VULKAN(result);
    }
} // namespace vkBasalt
