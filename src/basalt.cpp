#include "layer_state.hpp"

#include <vkbasalt_export.h>

#include <vulkan/vulkan_core.h>

extern "C"
{ // these are the entry points for the layer, so they need to be c-linkeable
    VKBASALT_EXPORT PFN_vkVoidFunction VKAPI_CALL vkBasalt_GetDeviceProcAddr(VkDevice device, const char* pName)
    {
        return vkBasalt::LayerState::GetDeviceProcAddr(device, pName);
    }

    VKBASALT_EXPORT PFN_vkVoidFunction VKAPI_CALL vkBasalt_GetInstanceProcAddr(VkInstance instance, const char* pName)
    {
        return vkBasalt::LayerState::GetInstanceProcAddr(instance, pName);
    }
} // extern "C"
