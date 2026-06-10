#ifndef SHADER_HPP_INCLUDED
#define SHADER_HPP_INCLUDED

#include <span>
#include <cstdint>
#include <vulkan/vulkan_core.h>

#include "logical_device.hpp"

namespace vkBasalt
{
    void createShaderModule(LogicalDevice* pLogicalDevice, std::span<const uint32_t> code, VkShaderModule* shaderModule);
} // namespace vkBasalt

#endif // SHADER_HPP_INCLUDED
