#ifndef VKBASALT_VKDISPACTCH_HPP
#define VKBASALT_VKDISPACTCH_HPP

#include "vkfuncs.hpp"

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{

#define FORVKFUNC(func) PFN_vk##func func = nullptr;
    struct InstanceDispatch
    {
        FORVKFUNC(GetInstanceProcAddr)
        VK_INSTANCE_FUNCS
    };

    struct DeviceDispatch
    {
        FORVKFUNC(GetDeviceProcAddr)
        VK_DEVICE_FUNCS
    };
#undef FORVKFUNC

    // fetch our own dispatch table for the functions we need, into the next layer
    inline InstanceDispatch fillDispatchTableInstance(VkInstance instance, PFN_vkGetInstanceProcAddr gipa) noexcept
    {
#define FORVKFUNC(func) .func = (PFN_vk##func) gipa(instance, "vk" #func),
        return {.GetInstanceProcAddr = gipa, VK_INSTANCE_FUNCS};
#undef FORVKFUNC
    }

    inline DeviceDispatch fillDispatchTableDevice(VkDevice device, PFN_vkGetDeviceProcAddr gdpa) noexcept
    {
#define FORVKFUNC(func) .func = (PFN_vk##func) gdpa(device, "vk" #func),
        return {.GetDeviceProcAddr = gdpa, VK_DEVICE_FUNCS};
#undef FORVKFUNC
    }

} // namespace vkBasalt

#undef VK_INSTANCE_FUNCS
#undef VK_DEVICE_FUNCS

#endif // VKBASALT_VKDISPACTCH_HPP