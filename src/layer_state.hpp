#ifndef LAYER_STATE_HPP
#define LAYER_STATE_HPP

#include "vkdispatch.hpp"
#include "logical_swapchain.hpp"
#include "config.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <unordered_map>

#include <vulkan/vulkan_core.h>

namespace vkBasalt
{
    class LayerState
    {
        struct InstanceData
        {
            InstanceDispatch dispatch{};
            uint32_t         version{};
        };

        std::unordered_map<void*, InstanceData>              instanceMap;
        std::unordered_map<void*, LogicalDevice>             deviceMap;
        std::unordered_map<VkSwapchainKHR, LogicalSwapchain> swapchainMap;

        std::mutex globalLock; // TODO: try std::shared_mutex
        Config     config;

        LayerState() = default;

        static LayerState& Get()
        {
            static LayerState state{};
            return state;
        }

        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // intercepted calls
        static VkResult VKAPI_CALL CreateInstance(const VkInstanceCreateInfo*  pCreateInfo,
                                                  const VkAllocationCallbacks* pAllocator,
                                                  VkInstance*                  pInstance);

        static void VKAPI_CALL DestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator);

        static VkResult VKAPI_CALL CreateDevice(VkPhysicalDevice             physicalDevice,
                                                const VkDeviceCreateInfo*    pCreateInfo,
                                                const VkAllocationCallbacks* pAllocator,
                                                VkDevice*                    pDevice);

        static void VKAPI_CALL DestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator);

        static VKAPI_ATTR VkResult VKAPI_CALL CreateSwapchainKHR(VkDevice                        device,
                                                                 const VkSwapchainCreateInfoKHR* pCreateInfo,
                                                                 const VkAllocationCallbacks*    pAllocator,
                                                                 VkSwapchainKHR*                 pSwapchain);

        static VKAPI_ATTR VkResult VKAPI_CALL GetSwapchainImagesKHR(VkDevice       device,
                                                                    VkSwapchainKHR swapchain,
                                                                    uint32_t*      pCount,
                                                                    VkImage*       pSwapchainImages);

        static VKAPI_ATTR VkResult VKAPI_CALL QueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo);

        static VKAPI_ATTR void VKAPI_CALL DestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator);

        static VKAPI_ATTR VkResult VKAPI_CALL CreateImage(VkDevice                     device,
                                                          const VkImageCreateInfo*     pCreateInfo,
                                                          const VkAllocationCallbacks* pAllocator,
                                                          VkImage*                     pImage);

        static VKAPI_ATTR VkResult VKAPI_CALL BindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset);

        static VKAPI_ATTR void VKAPI_CALL DestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator);

        // Enumeration function
        static VkResult VKAPI_CALL EnumerateInstanceLayerProperties(uint32_t* pPropertyCount, VkLayerProperties* pProperties);

        static VkResult VKAPI_CALL EnumerateDeviceLayerProperties(VkPhysicalDevice /*physicalDevice*/,
                                                                  uint32_t*          pPropertyCount,
                                                                  VkLayerProperties* pProperties);

        static VkResult VKAPI_CALL EnumerateInstanceExtensionProperties(const char* pLayerName,
                                                                        uint32_t*   pPropertyCount,
                                                                        VkExtensionProperties* /*pProperties*/);

        static VkResult VKAPI_CALL EnumerateDeviceExtensionProperties(VkPhysicalDevice       physicalDevice,
                                                                      const char*            pLayerName,
                                                                      uint32_t*              pPropertyCount,
                                                                      VkExtensionProperties* pProperties);

        static std::optional<PFN_vkVoidFunction> InterceptedCalls(std::string_view procName);

    public:
        LayerState(const LayerState&)            = delete;
        LayerState& operator=(const LayerState&) = delete;
        LayerState(LayerState&&)                 = delete;
        LayerState& operator=(LayerState&&)      = delete;
        ~LayerState()                            = default;

        // The only public methods that must be called in corresponding functions in export "C" block
        static PFN_vkVoidFunction VKAPI_CALL GetDeviceProcAddr(VkDevice device, const char* pName);
        static PFN_vkVoidFunction VKAPI_CALL GetInstanceProcAddr(VkInstance instance, const char* pName);
    };
} // namespace vkBasalt

#endif // LAYER_STATE_HPP
