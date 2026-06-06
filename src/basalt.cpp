#include "vulkan_include.hpp"
#include "vkdispatch.hpp"
#include "util.hpp"
#include "keyboard_input.hpp"
#include "logical_device.hpp"
#include "logical_swapchain.hpp"
#include "image_view.hpp"
#include "command_buffer.hpp"
#include "config.hpp"
#include "fake_swapchain.hpp"
#include "format.hpp"
#include "logger.hpp"
#include "effect_fxaa.hpp"
#include "effect_cas.hpp"
#include "effect_dls.hpp"
#include "effect_smaa.hpp"
#include "effect_deband.hpp"
#include "effect_lut.hpp"
#include "effect_reshade.hpp"
#include "effect_transfer.hpp"

#include <algorithm>
#include <array>
#include <iterator>
#include <mutex>
#include <map>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <utility>
#include <cstdint>

#include <vkbasalt_export.h>

#include <vulkan/vulkan_core.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vk_layer.h>

using namespace std::string_view_literals;

constexpr static auto vkBasaltVkLayerName{"VK_LAYER_VKBASALT_post_processing"sv};

namespace vkBasalt
{
    Logger Logger::s_instance;

    namespace
    {
        template<typename DispatchableType>
        void* GetKey(DispatchableType inst)
        {
            return *(void**) inst;
        }

        // TODO: better name for this singleton
        class GlobalState
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

            GlobalState() = default;

            static GlobalState& Get()
            {
                static GlobalState state{};
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
            GlobalState(const GlobalState&)            = delete;
            GlobalState& operator=(const GlobalState&) = delete;
            GlobalState(GlobalState&&)                 = delete;
            GlobalState& operator=(GlobalState&&)      = delete;
            ~GlobalState()                             = default;

            // The only public methods that must be called in corresponding functions in export "C" block
            static PFN_vkVoidFunction VKAPI_CALL GetDeviceProcAddr(VkDevice device, const char* pName);
            static PFN_vkVoidFunction VKAPI_CALL GetInstanceProcAddr(VkInstance instance, const char* pName);
        };

        VkResult VKAPI_CALL GlobalState::CreateInstance(const VkInstanceCreateInfo*  pCreateInfo,
                                                        const VkAllocationCallbacks* pAllocator,
                                                        VkInstance*                  pInstance)
        {
            auto* layerCreateInfo = (VkLayerInstanceCreateInfo*) pCreateInfo->pNext;

            // step through the chain of pNext until we get to the link info
            while (layerCreateInfo != nullptr
                   && (layerCreateInfo->sType != VkStructureType::VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO
                       || layerCreateInfo->function != VkLayerFunction::VK_LAYER_LINK_INFO))
            {
                layerCreateInfo = (VkLayerInstanceCreateInfo*) layerCreateInfo->pNext;
            }

            Logger::trace("vkCreateInstance");

            if (layerCreateInfo == nullptr)
            {
                // No loader instance create info
                return VkResult::VK_ERROR_INITIALIZATION_FAILED;
            }

            auto gpa = layerCreateInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr;
            // move chain on for next layer
            layerCreateInfo->u.pLayerInfo = layerCreateInfo->u.pLayerInfo->pNext;

            auto createFunc = reinterpret_cast<PFN_vkCreateInstance>(gpa(VK_NULL_HANDLE, "vkCreateInstance"));

            auto              modifiedCreateInfo = *pCreateInfo;
            VkApplicationInfo appInfo;
            if (modifiedCreateInfo.pApplicationInfo != nullptr)
            {
                appInfo            = *(modifiedCreateInfo.pApplicationInfo);
                appInfo.apiVersion = std::max(appInfo.apiVersion, VK_API_VERSION_1_1);
            }
            else
            {
                appInfo.sType              = VkStructureType::VK_STRUCTURE_TYPE_APPLICATION_INFO;
                appInfo.pNext              = nullptr;
                appInfo.pApplicationName   = nullptr;
                appInfo.applicationVersion = 0;
                appInfo.pEngineName        = nullptr;
                appInfo.engineVersion      = 0;
                appInfo.apiVersion         = VK_API_VERSION_1_1;
            }

            modifiedCreateInfo.pApplicationInfo = std::addressof(appInfo);
            const auto ret                      = createFunc(std::addressof(modifiedCreateInfo), pAllocator, pInstance);

            // fetch our own dispatch table for the functions we need, into the next layer
            InstanceDispatch dispatchTable;
            fillDispatchTableInstance(*pInstance, gpa, std::addressof(dispatchTable));

            // store the table by key
            {
                auto&                  state = GlobalState::Get();
                const std::scoped_lock lock{state.globalLock};
                state.instanceMap.emplace(GetKey(*pInstance),
                                          InstanceData{.dispatch = dispatchTable, .version = modifiedCreateInfo.pApplicationInfo->apiVersion});
            }

            return ret;
        }

        void VKAPI_CALL GlobalState::DestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator)
        {
            if (instance == nullptr)
            {
                Logger::err("null instance");
                return;
            }

            auto&                  state = GlobalState::Get();
            const std::scoped_lock lock{state.globalLock};

            Logger::trace("vkDestroyInstance");

            if (const auto node = state.instanceMap.extract(GetKey(instance)))
            {
                node.mapped().dispatch.DestroyInstance(instance, pAllocator);
            }
        }

        VkResult VKAPI_CALL GlobalState::CreateDevice(VkPhysicalDevice             physicalDevice,
                                                      const VkDeviceCreateInfo*    pCreateInfo,
                                                      const VkAllocationCallbacks* pAllocator,
                                                      VkDevice*                    pDevice)
        {
            auto&                  state = GlobalState::Get();
            const std::scoped_lock lock{state.globalLock};
            Logger::trace("vkCreateDevice");
            auto* layerCreateInfo = (VkLayerDeviceCreateInfo*) pCreateInfo->pNext;

            // step through the chain of pNext until we get to the link info
            while (layerCreateInfo != nullptr
                   && (layerCreateInfo->sType != VkStructureType::VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO
                       || layerCreateInfo->function != VkLayerFunction::VK_LAYER_LINK_INFO))
            {
                layerCreateInfo = (VkLayerDeviceCreateInfo*) layerCreateInfo->pNext;
            }

            if (layerCreateInfo == nullptr)
            {
                // No loader instance create info
                return VkResult::VK_ERROR_INITIALIZATION_FAILED;
            }

            PFN_vkGetInstanceProcAddr gipa = layerCreateInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr;
            PFN_vkGetDeviceProcAddr   gdpa = layerCreateInfo->u.pLayerInfo->pfnNextGetDeviceProcAddr;
            // move chain on for next layer
            layerCreateInfo->u.pLayerInfo = layerCreateInfo->u.pLayerInfo->pNext;

            auto createFunc = reinterpret_cast<PFN_vkCreateDevice>(gipa(VK_NULL_HANDLE, "vkCreateDevice"));

            // check and activate extentions
            uint32_t extensionCount = 0;

            const auto instanceIt{state.instanceMap.find(GetKey(physicalDevice))};
            if (instanceIt == std::cend(state.instanceMap))
            {
                // No Instance Dispatch for Physical Device
                return VkResult::VK_ERROR_INITIALIZATION_FAILED;
            }

            const auto& [instanceKey, instanceData] = *instanceIt;

            instanceData.dispatch.EnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
            std::vector<VkExtensionProperties> extensionProperties(extensionCount);
            instanceData.dispatch.EnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, std::data(extensionProperties));

            bool supportsMutableFormat = false;
            for (VkExtensionProperties properties : extensionProperties)
            {
                if (std::string_view{properties.extensionName} == "VK_KHR_swapchain_mutable_format")
                {
                    Logger::debug("device supports VK_KHR_swapchain_mutable_format");
                    supportsMutableFormat = true;
                    break;
                }
            }

            VkPhysicalDeviceProperties deviceProps;
            instanceData.dispatch.GetPhysicalDeviceProperties(physicalDevice, &deviceProps);

            VkDeviceCreateInfo       modifiedCreateInfo = *pCreateInfo;
            std::vector<const char*> enabledExtensionNames;
            if (modifiedCreateInfo.enabledExtensionCount != 0U)
            {
                enabledExtensionNames.assign(modifiedCreateInfo.ppEnabledExtensionNames,
                                             std::next(modifiedCreateInfo.ppEnabledExtensionNames, modifiedCreateInfo.enabledExtensionCount));
            }

            if (supportsMutableFormat && not std::ranges::contains(enabledExtensionNames, "VK_KHR_swapchain_mutable_format"))
            {
                Logger::debug("activating mutable_format");
                enabledExtensionNames.emplace_back("VK_KHR_swapchain_mutable_format");
            }

            if ((deviceProps.apiVersion < VK_API_VERSION_1_2 || instanceData.version < VK_API_VERSION_1_2)
                && std::ranges::contains(enabledExtensionNames, "VK_KHR_image_format_list"))
            {
                Logger::debug("activating image_format_list");
                enabledExtensionNames.emplace_back("VK_KHR_image_format_list");
            }

            modifiedCreateInfo.ppEnabledExtensionNames = std::data(enabledExtensionNames);
            modifiedCreateInfo.enabledExtensionCount   = std::size(enabledExtensionNames);

            // Active needed Features
            VkPhysicalDeviceFeatures deviceFeatures = {};
            if (modifiedCreateInfo.pEnabledFeatures != nullptr)
            {
                deviceFeatures = *(modifiedCreateInfo.pEnabledFeatures);
            }
            deviceFeatures.shaderImageGatherExtended = VK_TRUE;
            modifiedCreateInfo.pEnabledFeatures      = std::addressof(deviceFeatures);

            const auto ret = createFunc(physicalDevice, std::addressof(modifiedCreateInfo), pAllocator, pDevice);

            if (ret != VkResult::VK_SUCCESS)
            {
                return ret;
            }

            LogicalDevice logicalDevice{.vkd                   = {},
                                        .vki                   = instanceData.dispatch,
                                        .device                = *pDevice,
                                        .physicalDevice        = physicalDevice,
                                        .instance              = static_cast<VkInstance>(instanceKey),
                                        .queue                 = VK_NULL_HANDLE,
                                        .queueFamilyIndex      = 0,
                                        .commandPool           = VK_NULL_HANDLE,
                                        .supportsMutableFormat = supportsMutableFormat,
                                        .depthImages           = {},
                                        .depthFormats          = {},
                                        .depthImageViews       = {}};

            fillDispatchTableDevice(*pDevice, gdpa, &logicalDevice.vkd);

            uint32_t count{};
            logicalDevice.vki.GetPhysicalDeviceQueueFamilyProperties(logicalDevice.physicalDevice, &count, nullptr);

            std::vector<VkQueueFamilyProperties> queueProperties(count);

            logicalDevice.vki.GetPhysicalDeviceQueueFamilyProperties(logicalDevice.physicalDevice, &count, queueProperties.data());
            for (const auto& queueInfo : std::span{pCreateInfo->pQueueCreateInfos, pCreateInfo->queueCreateInfoCount})
            {
                if ((queueProperties[queueInfo.queueFamilyIndex].queueFlags & VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT) != 0U)
                {
                    logicalDevice.vkd.GetDeviceQueue(logicalDevice.device, queueInfo.queueFamilyIndex, 0, &logicalDevice.queue);

                    VkCommandPoolCreateInfo commandPoolCreateInfo;
                    commandPoolCreateInfo.sType            = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                    commandPoolCreateInfo.pNext            = nullptr;
                    commandPoolCreateInfo.flags            = 0;
                    commandPoolCreateInfo.queueFamilyIndex = queueInfo.queueFamilyIndex;

                    Logger::debug("Found graphics capable queue");
                    logicalDevice.vkd.CreateCommandPool(logicalDevice.device, &commandPoolCreateInfo, nullptr, &logicalDevice.commandPool);
                    logicalDevice.queueFamilyIndex = queueInfo.queueFamilyIndex;

                    initializeDispatchTable(logicalDevice.queue, logicalDevice.device);

                    break;
                }
            }

            if (logicalDevice.queue == nullptr)
            {
                Logger::err("Did not find a graphics queue!");
            }

            state.deviceMap.emplace(GetKey(*pDevice), std::move(logicalDevice));

            return VkResult::VK_SUCCESS;
        }

        void VKAPI_CALL GlobalState::DestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator)
        {
            if (device == nullptr)
            {
                Logger::err("null device");
                return;
            }

            auto&                  state = GlobalState::Get();
            const std::scoped_lock lock{state.globalLock};

            Logger::trace("vkDestroyDevice");

            // TODO: extract node instead
            const auto deviceIt{state.deviceMap.find(GetKey(device))};
            if (deviceIt == std::cend(state.deviceMap))
            {
                Logger::err("could not find device in map");
                return;
            }

            auto& [key, logicalDevice] = *deviceIt;
            if (logicalDevice.commandPool != VK_NULL_HANDLE)
            {
                Logger::debug("DestroyCommandPool");
                logicalDevice.vkd.DestroyCommandPool(device, logicalDevice.commandPool, pAllocator);
            }

            logicalDevice.vkd.DestroyDevice(device, pAllocator);

            state.deviceMap.erase(GetKey(device));
        }

        VKAPI_ATTR VkResult VKAPI_CALL GlobalState::CreateSwapchainKHR(VkDevice                        device,
                                                                       const VkSwapchainCreateInfoKHR* pCreateInfo,
                                                                       const VkAllocationCallbacks*    pAllocator,
                                                                       VkSwapchainKHR*                 pSwapchain)
        {
            auto&                  state = GlobalState::Get();
            const std::scoped_lock lock{state.globalLock};

            Logger::trace("vkCreateSwapchainKHR");

            const auto deviceIt{state.deviceMap.find(GetKey(device))};
            if (deviceIt == std::cend(state.deviceMap))
            {
                return VkResult::VK_ERROR_UNKNOWN;
            }
            auto& [key, logicalDevice] = *deviceIt;

            VkSwapchainCreateInfoKHR modifiedCreateInfo = *pCreateInfo;

            const auto format = modifiedCreateInfo.imageFormat;

            const auto srgbFormat  = isSRGB(format) ? format : convertToSRGB(format);
            const auto unormFormat = isSRGB(format) ? convertToUNORM(format) : format;
            Logger::debug(std::to_string(srgbFormat) + " " + std::to_string(unormFormat));

            const std::array formats{unormFormat, srgbFormat};

            VkImageFormatListCreateInfoKHR imageFormatListCreateInfo;
            if (logicalDevice.supportsMutableFormat)
            {
                modifiedCreateInfo.imageUsage =
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                    | VK_IMAGE_USAGE_SAMPLED_BIT; // we want to use the swapchain images as output of the graphics pipeline
                modifiedCreateInfo.flags |= VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR;
                // TODO what if the application already uses multiple formats for the swapchain?

                imageFormatListCreateInfo.sType           = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR;
                imageFormatListCreateInfo.pNext           = modifiedCreateInfo.pNext;
                imageFormatListCreateInfo.viewFormatCount = (srgbFormat == unormFormat) ? 1 : 2;
                imageFormatListCreateInfo.pViewFormats    = std::data(formats);

                modifiedCreateInfo.pNext = std::addressof(imageFormatListCreateInfo);
            }

            modifiedCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

            Logger::debug("format " + std::to_string(modifiedCreateInfo.imageFormat));

            const auto result = logicalDevice.vkd.CreateSwapchainKHR(device, &modifiedCreateInfo, pAllocator, pSwapchain);

            // TODO: check success on emplace
            state.swapchainMap.emplace(*pSwapchain,
                                       LogicalSwapchain{.pLogicalDevice         = std::addressof(logicalDevice),
                                                        .swapchainCreateInfo    = *pCreateInfo,
                                                        .imageExtent            = modifiedCreateInfo.imageExtent,
                                                        .format                 = modifiedCreateInfo.imageFormat,
                                                        .imageCount             = 0,
                                                        .images                 = {},
                                                        .fakeImages             = {},
                                                        .commandBuffersEffect   = {},
                                                        .commandBuffersNoEffect = {},
                                                        .semaphores             = {},
                                                        .effects                = {},
                                                        .defaultTransfer        = {},
                                                        .fakeImageMemory        = {}});

            return result;
        }

        VKAPI_ATTR VkResult VKAPI_CALL GlobalState::GetSwapchainImagesKHR(VkDevice       device,
                                                                          VkSwapchainKHR swapchain,
                                                                          uint32_t*      pCount,
                                                                          VkImage*       pSwapchainImages)
        {
            auto&                  state = GlobalState::Get();
            const std::scoped_lock lock{state.globalLock};
            Logger::trace("vkGetSwapchainImagesKHR " + std::to_string(*pCount));

            const auto deviceIt{state.deviceMap.find(GetKey(device))};
            if (deviceIt == std::cend(state.deviceMap))
            {
                Logger::err("could not find device in map");
                return VkResult::VK_ERROR_UNKNOWN;
            }
            auto& [key, logicalDevice] = *deviceIt;

            if (pSwapchainImages == nullptr)
            {
                return logicalDevice.vkd.GetSwapchainImagesKHR(device, swapchain, pCount, pSwapchainImages);
            }

            const auto logicalSwapchainIt = state.swapchainMap.find(swapchain);
            if (logicalSwapchainIt == std::cend(state.swapchainMap))
            {
                Logger::err("could not find device in map");
                return VkResult::VK_ERROR_UNKNOWN;
            }
            auto& logicalSwapchain = logicalSwapchainIt->second;

            // If the images got already requested once, return them again instead of creating new images
            if (not std::empty(logicalSwapchain.fakeImages))
            {
                *pCount = std::min<uint32_t>(*pCount, logicalSwapchain.imageCount);
                std::ranges::copy_n(std::cbegin(logicalSwapchain.fakeImages), *pCount, pSwapchainImages);
                return *pCount < logicalSwapchain.imageCount ? VkResult::VK_INCOMPLETE : VkResult::VK_SUCCESS;
            }

            logicalDevice.vkd.GetSwapchainImagesKHR(device, swapchain, &logicalSwapchain.imageCount, nullptr);
            logicalSwapchain.images.resize(logicalSwapchain.imageCount);
            logicalDevice.vkd.GetSwapchainImagesKHR(device, swapchain, &logicalSwapchain.imageCount, std::data(logicalSwapchain.images));

            auto effectStrings = state.config.getOption<std::vector<std::string>>("effects", {"cas"});

            // create 1 more set of images when we can't use the swapchain it self
            const auto fakeImageCount = logicalSwapchain.imageCount * (std::size(effectStrings) + (!logicalDevice.supportsMutableFormat ? 1U : 0U));

            logicalSwapchain.fakeImages = createFakeSwapchainImages(
                std::addressof(logicalDevice), logicalSwapchain.swapchainCreateInfo, fakeImageCount, logicalSwapchain.fakeImageMemory);
            Logger::debug("created fake swapchain images");

            const auto unormFormat = convertToUNORM(logicalSwapchain.format);
            const auto srgbFormat  = convertToSRGB(logicalSwapchain.format);

            logicalSwapchain.fakeImages | std::views::chunk(logicalSwapchain.imageCount);

            for (uint32_t i = 0; i < std::size(effectStrings); ++i)
            {
                Logger::debug("current effectString " + effectStrings[i]);
                std::vector<VkImage> firstImages(std::next(std::begin(logicalSwapchain.fakeImages), logicalSwapchain.imageCount * i),
                                                 std::next(std::begin(logicalSwapchain.fakeImages), logicalSwapchain.imageCount * (i + 1)));
                Logger::debug(std::to_string(std::size(firstImages)) + " images in firstImages");
                std::vector<VkImage> secondImages;
                // for last element of effectStrings
                if (std::size(effectStrings) == i + 1U)
                {
                    logicalDevice.supportsMutableFormat
                        ? secondImages.assign(std::cbegin(logicalSwapchain.images), std::cend(logicalSwapchain.images))
                        : secondImages.assign(std::prev(std::cend(logicalSwapchain.fakeImages), logicalSwapchain.imageCount),
                                              std::cend(logicalSwapchain.fakeImages));
                    Logger::debug("using swapchain images as second images");
                }
                else
                {
                    secondImages.assign(std::next(std::begin(logicalSwapchain.fakeImages), logicalSwapchain.imageCount * (i + 1)),
                                        std::next(std::begin(logicalSwapchain.fakeImages), logicalSwapchain.imageCount * (i + 2)));
                    Logger::debug("not using swapchain images as second images");
                }
                Logger::debug(std::to_string(std::size(secondImages)) + " images in secondImages");
                // TODO: in separate function
                if (effectStrings[i] == "fxaa")
                {
                    logicalSwapchain.effects.emplace_back(std::make_unique<FxaaEffect>(std::addressof(logicalDevice),
                                                                                       srgbFormat,
                                                                                       logicalSwapchain.imageExtent,
                                                                                       firstImages,
                                                                                       secondImages,
                                                                                       std::addressof(state.config)));
                    Logger::debug("created FxaaEffect");
                }
                else if (effectStrings[i] == "cas")
                {
                    logicalSwapchain.effects.emplace_back(std::make_unique<CasEffect>(std::addressof(logicalDevice),
                                                                                      unormFormat,
                                                                                      logicalSwapchain.imageExtent,
                                                                                      firstImages,
                                                                                      secondImages,
                                                                                      std::addressof(state.config)));
                    Logger::debug("created CasEffect");
                }
                else if (effectStrings[i] == "deband")
                {
                    logicalSwapchain.effects.emplace_back(std::make_unique<DebandEffect>(std::addressof(logicalDevice),
                                                                                         unormFormat,
                                                                                         logicalSwapchain.imageExtent,
                                                                                         firstImages,
                                                                                         secondImages,
                                                                                         std::addressof(state.config)));
                    Logger::debug("created DebandEffect");
                }
                else if (effectStrings[i] == "smaa")
                {
                    logicalSwapchain.effects.emplace_back(std::make_unique<SmaaEffect>(std::addressof(logicalDevice),
                                                                                       unormFormat,
                                                                                       logicalSwapchain.imageExtent,
                                                                                       firstImages,
                                                                                       secondImages,
                                                                                       std::addressof(state.config)));
                    Logger::debug("created SmaaEffect");
                }
                else if (effectStrings[i] == "lut")
                {
                    logicalSwapchain.effects.emplace_back(std::make_unique<LutEffect>(std::addressof(logicalDevice),
                                                                                      unormFormat,
                                                                                      logicalSwapchain.imageExtent,
                                                                                      firstImages,
                                                                                      secondImages,
                                                                                      std::addressof(state.config)));
                    Logger::debug("created LutEffect");
                }
                else if (effectStrings[i] == "dls")
                {
                    logicalSwapchain.effects.emplace_back(std::make_unique<DlsEffect>(std::addressof(logicalDevice),
                                                                                      unormFormat,
                                                                                      logicalSwapchain.imageExtent,
                                                                                      firstImages,
                                                                                      secondImages,
                                                                                      std::addressof(state.config)));
                    Logger::debug("created DlsEffect");
                }
                else
                {
                    logicalSwapchain.effects.emplace_back(std::make_unique<ReshadeEffect>(std::addressof(logicalDevice),
                                                                                          logicalSwapchain.format,
                                                                                          logicalSwapchain.imageExtent,
                                                                                          firstImages,
                                                                                          secondImages,
                                                                                          std::addressof(state.config),
                                                                                          effectStrings[i]));
                    Logger::debug("created ReshadeEffect");
                }
            }

            // Below does not depend on index

            if (!logicalDevice.supportsMutableFormat)
            {
                logicalSwapchain.effects.emplace_back(std::make_unique<TransferEffect>(
                    std::addressof(logicalDevice),
                    logicalSwapchain.format,
                    logicalSwapchain.imageExtent,
                    std::span{std::prev(std::end(logicalSwapchain.fakeImages), logicalSwapchain.imageCount), std::end(logicalSwapchain.fakeImages)},
                    logicalSwapchain.images,
                    std::addressof(state.config)));
            }

            auto* depthImage     = std::empty(logicalDevice.depthImages) ? VK_NULL_HANDLE : logicalDevice.depthImages.front();
            auto* depthImageView = std::empty(logicalDevice.depthImageViews) ? VK_NULL_HANDLE : logicalDevice.depthImageViews.front();
            auto  depthFormat    = std::empty(logicalDevice.depthFormats) ? VK_FORMAT_UNDEFINED : logicalDevice.depthFormats.front();

            Logger::debug("effect string count: " + std::to_string(std::size(effectStrings)));
            Logger::debug("effect count: " + std::to_string(std::size(logicalSwapchain.effects)));

            logicalSwapchain.commandBuffersEffect = allocateCommandBuffer(std::addressof(logicalDevice), logicalSwapchain.imageCount);
            Logger::debug("allocated ComandBuffers " + std::to_string(std::size(logicalSwapchain.commandBuffersEffect)) + " for swapchain "
                          + convertToString(swapchain));

            writeCommandBuffers(std::addressof(logicalDevice),
                                logicalSwapchain.effects,
                                depthImage,
                                depthImageView,
                                depthFormat,
                                logicalSwapchain.commandBuffersEffect);
            Logger::debug("wrote CommandBuffers");

            logicalSwapchain.semaphores = createSemaphores(std::addressof(logicalDevice), logicalSwapchain.imageCount);
            Logger::debug("created semaphores");
            for (unsigned int i = 0; i < logicalSwapchain.imageCount; ++i)
            {
                Logger::debug(std::to_string(i) + " written commandbuffer " + convertToString(logicalSwapchain.commandBuffersEffect[i]));
            }
            Logger::trace("vkGetSwapchainImagesKHR");

            logicalSwapchain.defaultTransfer = std::make_unique<TransferEffect>(
                std::addressof(logicalDevice),
                logicalSwapchain.format,
                logicalSwapchain.imageExtent,
                std::span{std::begin(logicalSwapchain.fakeImages), std::next(std::begin(logicalSwapchain.fakeImages), logicalSwapchain.imageCount)},
                std::span{logicalSwapchain.images},
                std::addressof(state.config));

            logicalSwapchain.commandBuffersNoEffect = allocateCommandBuffer(std::addressof(logicalDevice), logicalSwapchain.imageCount);

            writeCommandBuffers(std::addressof(logicalDevice),
                                std::span{std::addressof(logicalSwapchain.defaultTransfer), 1U},
                                VK_NULL_HANDLE,
                                VK_NULL_HANDLE,
                                VkFormat::VK_FORMAT_UNDEFINED,
                                logicalSwapchain.commandBuffersNoEffect);

            for (unsigned int i = 0; i < logicalSwapchain.imageCount; i++)
            {
                Logger::debug(std::to_string(i) + " written commandbuffer " + convertToString(logicalSwapchain.commandBuffersNoEffect[i]));
            }

            *pCount = std::min<uint32_t>(*pCount, logicalSwapchain.imageCount);
            std::ranges::copy_n(std::cbegin(logicalSwapchain.fakeImages), *pCount, pSwapchainImages);

            return *pCount < logicalSwapchain.imageCount ? VkResult::VK_INCOMPLETE : VkResult::VK_SUCCESS;
        }

        VKAPI_ATTR VkResult VKAPI_CALL GlobalState::QueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo)
        {
            auto&                  state = GlobalState::Get();
            const std::scoped_lock lock{state.globalLock};

            const static uint32_t keySymbol = convertToKeySym(state.config.getOption<std::string>("toggleKey", "Home"));

            static bool pressed       = false;
            static bool presentEffect = state.config.getOption<bool>("enableOnLaunch", true);

            if (isKeyPressed(keySymbol))
            {
                if (!pressed)
                {
                    presentEffect = !presentEffect;
                    pressed       = true;
                }
            }
            else
            {
                pressed = false;
            }

            const auto deviceIt{state.deviceMap.find(GetKey(queue))};
            if (deviceIt == std::cend(state.deviceMap))
            {
                return VkResult::VK_ERROR_UNKNOWN;
            }
            auto& [key, logicalDevice] = *deviceIt;

            std::vector<VkSemaphore> presentSemaphores;
            presentSemaphores.reserve(pPresentInfo->swapchainCount);

            std::vector<VkPipelineStageFlags> waitStages(pPresentInfo->waitSemaphoreCount,
                                                         VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

            for (unsigned int i = 0; i < pPresentInfo->swapchainCount; i++)
            {
                const uint32_t index     = pPresentInfo->pImageIndices[i];
                VkSwapchainKHR swapchain = pPresentInfo->pSwapchains[i];

                const auto logicalSwapchainIt = state.swapchainMap.find(swapchain);
                if (logicalSwapchainIt == std::cend(state.swapchainMap))
                {
                    Logger::err("could not find swapchain in map");
                    continue;
                }
                auto& logicalSwapchain = logicalSwapchainIt->second;

                for (auto& effect : logicalSwapchain.effects)
                {
                    effect->updateEffect();
                }

                VkSubmitInfo submitInfo;
                submitInfo.sType                = VkStructureType::VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.pNext              = nullptr;
                submitInfo.waitSemaphoreCount   = (i == 0) ? pPresentInfo->waitSemaphoreCount : 0;
                submitInfo.pWaitSemaphores      = (i == 0) ? pPresentInfo->pWaitSemaphores : nullptr;
                submitInfo.pWaitDstStageMask    = (i == 0) ? std::data(waitStages) : nullptr;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers      = presentEffect ? std::addressof(logicalSwapchain.commandBuffersEffect[index])
                                                                : std::addressof(logicalSwapchain.commandBuffersNoEffect[index]);
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores    = std::addressof(logicalSwapchain.semaphores[index]);

                presentSemaphores.emplace_back(logicalSwapchain.semaphores[index]);

                const auto res = logicalDevice.vkd.QueueSubmit(logicalDevice.queue, 1, std::addressof(submitInfo), VK_NULL_HANDLE);

                if (res != VkResult::VK_SUCCESS)
                {
                    return res;
                }
            }

            VkPresentInfoKHR presentInfo   = *pPresentInfo;
            presentInfo.waitSemaphoreCount = presentSemaphores.size();
            presentInfo.pWaitSemaphores    = presentSemaphores.data();

            return logicalDevice.vkd.QueuePresentKHR(queue, &presentInfo);
        }

        VKAPI_ATTR void VKAPI_CALL GlobalState::DestroySwapchainKHR(VkDevice                     device,
                                                                    VkSwapchainKHR               swapchain,
                                                                    const VkAllocationCallbacks* pAllocator)
        {
            if (swapchain == nullptr)
            {
                Logger::err("null swapchain");
                return;
            }

            auto& state = GlobalState::Get();

            const std::scoped_lock lock{state.globalLock};
            // we need to delete the infos of the oldswapchain

            Logger::trace("vkDestroySwapchainKHR " + convertToString(swapchain));

            if (const auto node = state.swapchainMap.extract(swapchain); node)
            {
                node.mapped().destroy();
            }
            else
            {
                Logger::err("could not find swapchain in map");
            }

            if (const auto deviceIt{state.deviceMap.find(GetKey(device))}; deviceIt != std::cend(state.deviceMap))
            {
                deviceIt->second.vkd.DestroySwapchainKHR(device, swapchain, pAllocator);
            }
        }

        VKAPI_ATTR VkResult VKAPI_CALL GlobalState::CreateImage(VkDevice                     device,
                                                                const VkImageCreateInfo*     pCreateInfo,
                                                                const VkAllocationCallbacks* pAllocator,
                                                                VkImage*                     pImage)
        {
            auto&                  state = GlobalState::Get();
            const std::scoped_lock lock{state.globalLock};

            const auto deviceIt{state.deviceMap.find(GetKey(device))};
            if (deviceIt == std::cend(state.deviceMap))
            {
                return VkResult::VK_ERROR_UNKNOWN;
            }
            auto& [key, logicalDevice] = *deviceIt;

            if (isDepthFormat(pCreateInfo->format) && pCreateInfo->samples == VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT
                && ((pCreateInfo->usage & VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
                    == VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT))
            {
                Logger::debug("detected depth image with format: " + convertToString(pCreateInfo->format));
                Logger::debug(std::to_string(pCreateInfo->extent.width) + "x" + std::to_string(pCreateInfo->extent.height));
                Logger::debug(std::to_string(static_cast<int>((pCreateInfo->usage & VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
                                                              == VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)));

                VkImageCreateInfo modifiedCreateInfo = *pCreateInfo;
                modifiedCreateInfo.usage |= VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;
                const auto res = logicalDevice.vkd.CreateImage(device, &modifiedCreateInfo, pAllocator, pImage);
                logicalDevice.depthImages.emplace_back(*pImage);
                logicalDevice.depthFormats.emplace_back(pCreateInfo->format);

                return res;
            }

            return logicalDevice.vkd.CreateImage(device, pCreateInfo, pAllocator, pImage);
        }

        VKAPI_ATTR VkResult VKAPI_CALL GlobalState::BindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset)
        {
            auto&                  state = GlobalState::Get();
            const std::scoped_lock lock{state.globalLock};

            const auto deviceIt{state.deviceMap.find(GetKey(device))};
            if (deviceIt == std::cend(state.deviceMap))
            {
                return VkResult::VK_ERROR_UNKNOWN;
            }
            auto& [key, logicalDevice] = *deviceIt;

            const auto res = logicalDevice.vkd.BindImageMemory(device, image, memory, memoryOffset);
            // TODO what if the application creates more than one image before binding memory?
            if (not std::empty(logicalDevice.depthImages) && image == logicalDevice.depthImages.back())
            {
                Logger::debug("before creating depth image view");
                VkImageView depthImageView = createImageViews(std::addressof(logicalDevice),
                                                              logicalDevice.depthFormats[std::size(logicalDevice.depthImages) - 1],
                                                              std::span{std::addressof(image), 1U},
                                                              VkImageViewType::VK_IMAGE_VIEW_TYPE_2D,
                                                              VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT)[0];

                const auto depthFormat = logicalDevice.depthFormats[logicalDevice.depthImages.size() - 1];

                Logger::debug("created depth image view");
                logicalDevice.depthImageViews.emplace_back(depthImageView);
                if (logicalDevice.depthImageViews.size() > 1)
                {
                    return res;
                }

                for (auto& [key, logicalSwapchain] : state.swapchainMap)
                {
                    if (logicalSwapchain.pLogicalDevice == std::addressof(logicalDevice))
                    {
                        if (not std::empty(logicalSwapchain.commandBuffersEffect))
                        {
                            logicalDevice.vkd.FreeCommandBuffers(logicalDevice.device,
                                                                 logicalDevice.commandPool,
                                                                 logicalSwapchain.commandBuffersEffect.size(),
                                                                 logicalSwapchain.commandBuffersEffect.data());
                            logicalSwapchain.commandBuffersEffect.clear();
                            logicalSwapchain.commandBuffersEffect = allocateCommandBuffer(std::addressof(logicalDevice), logicalSwapchain.imageCount);
                            Logger::debug("allocated CommandBuffers for swapchain " + convertToString(key));

                            writeCommandBuffers(std::addressof(logicalDevice),
                                                logicalSwapchain.effects,
                                                image,
                                                depthImageView,
                                                depthFormat,
                                                logicalSwapchain.commandBuffersEffect);
                            Logger::debug("wrote CommandBuffers");
                        }
                    }
                }
            }
            return res;
        }

        VKAPI_ATTR void VKAPI_CALL GlobalState::DestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator)
        {
            if (image == nullptr)
            {
                Logger::err("null image");
                return;
            }

            auto&                  state = GlobalState::Get();
            const std::scoped_lock lock{state.globalLock};

            const auto deviceIt{state.deviceMap.find(GetKey(device))};
            if (deviceIt == std::cend(state.deviceMap))
            {
                Logger::err("could not find device in map");
                return;
            }

            auto& [key, logicalDevice] = *deviceIt;

            for (uint32_t i = 0; i < logicalDevice.depthImages.size(); i++)
            {
                if (logicalDevice.depthImages[i] == image)
                {
                    logicalDevice.depthImages.erase(std::next(std::begin(logicalDevice.depthImages), i));
                    // TODO what if a image gets destroyed before binding memory?
                    if (std::size(logicalDevice.depthImageViews) >= (i + 1U))
                    {
                        logicalDevice.vkd.DestroyImageView(logicalDevice.device, logicalDevice.depthImageViews[i], nullptr);
                        logicalDevice.depthImageViews.erase(std::next(std::begin(logicalDevice.depthImageViews), i));
                    }
                    logicalDevice.depthFormats.erase(logicalDevice.depthFormats.begin() + i);

                    auto*      depthImageView = std::empty(logicalDevice.depthImageViews) ? VK_NULL_HANDLE : logicalDevice.depthImageViews.front();
                    auto*      depthImage     = std::empty(logicalDevice.depthImages) ? VK_NULL_HANDLE : logicalDevice.depthImages.front();
                    const auto depthFormat =
                        std::empty(logicalDevice.depthFormats) ? logicalDevice.depthFormats.front() : VkFormat::VK_FORMAT_UNDEFINED;

                    for (auto& [key, logicalSwapchain] : state.swapchainMap)
                    {
                        if (logicalSwapchain.pLogicalDevice == std::addressof(logicalDevice))
                        {
                            if (not std::empty(logicalSwapchain.commandBuffersEffect))
                            {
                                logicalDevice.vkd.FreeCommandBuffers(logicalDevice.device,
                                                                     logicalDevice.commandPool,
                                                                     logicalSwapchain.commandBuffersEffect.size(),
                                                                     logicalSwapchain.commandBuffersEffect.data());
                                logicalSwapchain.commandBuffersEffect.clear();
                                logicalSwapchain.commandBuffersEffect =
                                    allocateCommandBuffer(std::addressof(logicalDevice), logicalSwapchain.imageCount);
                                Logger::debug("allocated CommandBuffers for swapchain " + convertToString(key));

                                writeCommandBuffers(std::addressof(logicalDevice),
                                                    logicalSwapchain.effects,
                                                    depthImage,
                                                    depthImageView,
                                                    depthFormat,
                                                    logicalSwapchain.commandBuffersEffect);
                                Logger::debug("wrote CommandBuffers");
                            }
                        }
                    }
                }
            }

            logicalDevice.vkd.DestroyImage(logicalDevice.device, image, pAllocator);
        }

        ///////////////////////////////////////////////////////////////////////////////////////////
        // Enumeration function

        VkResult VKAPI_CALL GlobalState::EnumerateInstanceLayerProperties(uint32_t* pPropertyCount, VkLayerProperties* pProperties)
        {
            if (pPropertyCount != nullptr)
            {
                *pPropertyCount = 1;
            }

            // TODO: strange logic
            if (pProperties != nullptr)
            {
                static constexpr auto description{"a post processing layer"sv};
                static_assert(std::size(description) < VK_MAX_DESCRIPTION_SIZE);
                std::ranges::copy(description, std::data(pProperties->description));

                static_assert(std::size(vkBasaltVkLayerName) < VK_MAX_EXTENSION_NAME_SIZE);
                std::ranges::copy(vkBasaltVkLayerName, std::data(pProperties->layerName));

                pProperties->implementationVersion = 1;
                pProperties->specVersion           = VK_MAKE_VERSION(1, 2, 0);
            }

            return VkResult::VK_SUCCESS;
        }

        VkResult VKAPI_CALL GlobalState::EnumerateDeviceLayerProperties(VkPhysicalDevice /*physicalDevice*/,
                                                                        uint32_t*          pPropertyCount,
                                                                        VkLayerProperties* pProperties)
        {
            return GlobalState::EnumerateInstanceLayerProperties(pPropertyCount, pProperties);
        }

        VkResult VKAPI_CALL GlobalState::EnumerateInstanceExtensionProperties(const char* pLayerName,
                                                                              uint32_t*   pPropertyCount,
                                                                              VkExtensionProperties* /*pProperties*/)
        {
            if (pLayerName == nullptr || pLayerName == vkBasaltVkLayerName)
            {
                return VkResult::VK_ERROR_LAYER_NOT_PRESENT;
            }

            // don't expose any extensions
            if (pPropertyCount != nullptr)
            {
                *pPropertyCount = 0;
            }
            return VkResult::VK_SUCCESS;
        }

        VkResult VKAPI_CALL GlobalState::EnumerateDeviceExtensionProperties(VkPhysicalDevice       physicalDevice,
                                                                            const char*            pLayerName,
                                                                            uint32_t*              pPropertyCount,
                                                                            VkExtensionProperties* pProperties)
        {
            // pass through any queries that aren't to us
            if (pLayerName == nullptr || pLayerName == vkBasaltVkLayerName)
            {
                if (physicalDevice == VK_NULL_HANDLE)
                {
                    return VkResult::VK_SUCCESS;
                }

                auto&                  state = GlobalState::Get();
                const std::scoped_lock lock{state.globalLock};
                if (const auto instanceIt{state.instanceMap.find(GetKey(physicalDevice))}; instanceIt != std::cend(state.instanceMap))
                {
                    return instanceIt->second.dispatch.EnumerateDeviceExtensionProperties(physicalDevice, pLayerName, pPropertyCount, pProperties);
                }

                return VkResult::VK_ERROR_UNKNOWN;
            }

            // don't expose any extensions
            if (pPropertyCount != nullptr)
            {
                *pPropertyCount = 0;
            }
            return VkResult::VK_SUCCESS;
        }

        PFN_vkVoidFunction VKAPI_CALL GlobalState::GetDeviceProcAddr(VkDevice device, const char* pName)
        {
            // return overriden procedures
            return InterceptedCalls(pName)
                .or_else([device, pName]() -> std::optional<PFN_vkVoidFunction> {
                    // return proc from device dispatch table
                    auto&                  state = GlobalState::Get();
                    const std::scoped_lock lock{state.globalLock};
                    if (const auto deviceIt{state.deviceMap.find(vkBasalt::GetKey(device))}; deviceIt != std::cend(state.deviceMap))
                    {
                        return deviceIt->second.vkd.GetDeviceProcAddr(device, pName);
                    }
                    return std::nullopt;
                })
                .value_or(nullptr);
        }

        PFN_vkVoidFunction VKAPI_CALL GlobalState::GetInstanceProcAddr(VkInstance instance, const char* pName)
        {
            return InterceptedCalls(pName)
                .or_else([instance, pName]() -> std::optional<PFN_vkVoidFunction> {
                    auto&                  state = GlobalState::Get();
                    const std::scoped_lock lock(state.globalLock);
                    if (const auto instanceIt{state.instanceMap.find(vkBasalt::GetKey(instance))}; instanceIt != std::cend(state.instanceMap))
                    {
                        return instanceIt->second.dispatch.GetInstanceProcAddr(instance, pName);
                    }
                    return std::nullopt;
                })
                .value_or(nullptr);
        }

        std::optional<PFN_vkVoidFunction> GlobalState::InterceptedCalls(std::string_view procName)
        {
            // instance chain functions we intercept
            if (procName == "vkGetInstanceProcAddr")
            {
                return reinterpret_cast<PFN_vkVoidFunction>(std::addressof(GetInstanceProcAddr));
            }
            if (procName == "vkEnumerateInstanceLayerProperties")
            {
                return reinterpret_cast<PFN_vkVoidFunction>(std::addressof(EnumerateInstanceLayerProperties));
            }
            if (procName == "vkEnumerateInstanceExtensionProperties")
            {
                return reinterpret_cast<PFN_vkVoidFunction>(std::addressof(EnumerateInstanceExtensionProperties));
            }
            if (procName == "vkCreateInstance")
            {
                return reinterpret_cast<PFN_vkVoidFunction>(std::addressof(CreateInstance));
            }
            if (procName == "vkDestroyInstance")
            {
                return reinterpret_cast<PFN_vkVoidFunction>(std::addressof(DestroyInstance));
            }
            // device chain functions we intercept
            // vkGetDeviceProcAddr needs to behave like vkGetInstanceProcAddr thanks to some games
            if (procName == "vkGetDeviceProcAddr")
            {
                return reinterpret_cast<PFN_vkVoidFunction>(std::addressof(GetDeviceProcAddr));
            }
            if (procName == "vkEnumerateDeviceLayerProperties")
            {
                return reinterpret_cast<PFN_vkVoidFunction>(std::addressof(EnumerateDeviceLayerProperties));
            }
            if (procName == "vkEnumerateDeviceExtensionProperties")
            {
                return reinterpret_cast<PFN_vkVoidFunction>(std::addressof(EnumerateDeviceExtensionProperties));
            }
            if (procName == "vkCreateDevice")
            {
                return reinterpret_cast<PFN_vkVoidFunction>(std::addressof(CreateDevice));
            }
            if (procName == "vkDestroyDevice")
            {
                return reinterpret_cast<PFN_vkVoidFunction>(std::addressof(DestroyDevice));
            }
            if (procName == "vkCreateSwapchainKHR")
            {
                return reinterpret_cast<PFN_vkVoidFunction>(std::addressof(CreateSwapchainKHR));
            }
            if (procName == "vkGetSwapchainImagesKHR")
            {
                return reinterpret_cast<PFN_vkVoidFunction>(std::addressof(GetSwapchainImagesKHR));
            }
            if (procName == "vkQueuePresentKHR")
            {
                return reinterpret_cast<PFN_vkVoidFunction>(std::addressof(QueuePresentKHR));
            }
            if (procName == "vkDestroySwapchainKHR")
            {
                return reinterpret_cast<PFN_vkVoidFunction>(std::addressof(DestroySwapchainKHR));
            }

            if (auto& state = GlobalState::Get(); state.config.getOption<std::string>("depthCapture", "off") != "on")
            {
                return std::nullopt;
            }

            if (procName == "vkCreateImage")
            {
                return reinterpret_cast<PFN_vkVoidFunction>(std::addressof(CreateImage));
            }
            if (procName == "vkDestroyImage")
            {
                return reinterpret_cast<PFN_vkVoidFunction>(std::addressof(DestroyImage));
            }
            if (procName == "vkBindImageMemory")
            {
                return reinterpret_cast<PFN_vkVoidFunction>(std::addressof(BindImageMemory));
            }

            return std::nullopt;
        }
    } // namespace

} // namespace vkBasalt

extern "C"
{ // these are the entry points for the layer, so they need to be c-linkeable
    VKBASALT_EXPORT PFN_vkVoidFunction VKAPI_CALL vkBasalt_GetDeviceProcAddr(VkDevice device, const char* pName)
    {
        return vkBasalt::GlobalState::GetDeviceProcAddr(device, pName);
    }

    VKBASALT_EXPORT PFN_vkVoidFunction VKAPI_CALL vkBasalt_GetInstanceProcAddr(VkInstance instance, const char* pName)
    {
        return vkBasalt::GlobalState::GetInstanceProcAddr(instance, pName);
    }

} // extern "C"
