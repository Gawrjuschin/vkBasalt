#ifndef VULKAN_INCLUDE_HPP_INCLUDED
#define VULKAN_INCLUDE_HPP_INCLUDED

#include <string>
#define VK_NO_PROTOTYPES

#include <vulkan/vulkan_core.h>

#include <logger.hpp>

namespace vkBasalt
{

    inline void AssertVulkan(VkResult val)
    {
        if (val != VkResult::VK_SUCCESS)
        {
            Logger::err("ASSERT_VULKAN failed in " + std::string(__FILE__) + " : " + std::to_string(__LINE__) + "; " + std::to_string(val));
        }
    }

    template<typename DispatchableType, typename SuperDispatchableType>
    inline void initializeDispatchTable(DispatchableType dispatchableObject, SuperDispatchableType source)
    {
        *reinterpret_cast<void**>(dispatchableObject) = *reinterpret_cast<void**>(source);
    }
} // namespace vkBasalt

#endif // VULKAN_INCLUDE_HPP_INCLUDED
