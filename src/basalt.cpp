#include "vulkan_include.hpp"

#include <iterator>
#include <mutex>
#include <map>
#include <optional>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <cstring>

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

#include <vkbasalt_export.h>
#include <vulkan/vulkan_core.h>

using namespace std::string_view_literals;

constexpr static auto vkBasaltVkLayerName{"VK_LAYER_VKBASALT_post_processing"sv};

namespace vkBasalt
{
    Logger Logger::s_instance;

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
        std::unordered_map<void*, InstanceData>                               instanceMap;
        std::unordered_map<void*, LogicalDevice>                              deviceMap;
        std::unordered_map<VkSwapchainKHR, std::shared_ptr<LogicalSwapchain>> swapchainMap;

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
        static VkResult VKAPI_CALL            CreateInstance(const VkInstanceCreateInfo*  pCreateInfo,
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

        static VkResult VKAPI_CALL EnumerateDeviceLayerProperties(VkPhysicalDevice   physicalDevice,
                                                                  uint32_t*          pPropertyCount,
                                                                  VkLayerProperties* pProperties);

        static VkResult VKAPI_CALL EnumerateInstanceExtensionProperties(const char*            pLayerName,
                                                                        uint32_t*              pPropertyCount,
                                                                        VkExtensionProperties* pProperties);

        static VkResult VKAPI_CALL EnumerateDeviceExtensionProperties(VkPhysicalDevice       physicalDevice,
                                                                      const char*            pLayerName,
                                                                      uint32_t*              pPropertyCount,
                                                                      VkExtensionProperties* pProperties);

        static std::optional<PFN_vkVoidFunction> InterceptedCalls(std::string_view procName);

    public:
        GlobalState(const GlobalState&) = delete;
        GlobalState& operator=(const GlobalState&) = delete;
        GlobalState(GlobalState&&)      = delete;
        GlobalState& operator=(GlobalState&&)      = delete;
        ~GlobalState()                  = default;

        // The only public methods that must be called in corresponding functions in export "C" block
        static PFN_vkVoidFunction VKAPI_CALL GetDeviceProcAddr(VkDevice device, const char* pName);
        static PFN_vkVoidFunction VKAPI_CALL GetInstanceProcAddr(VkInstance instance, const char* pName);
    };

    VkResult VKAPI_CALL GlobalState::CreateInstance(const VkInstanceCreateInfo*  pCreateInfo,
                                                    const VkAllocationCallbacks* pAllocator,
                                                    VkInstance*                  pInstance)
    {
        VkLayerInstanceCreateInfo* layerCreateInfo = (VkLayerInstanceCreateInfo*) pCreateInfo->pNext;

        // step through the chain of pNext until we get to the link info
        while (layerCreateInfo
               && (layerCreateInfo->sType != VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO || layerCreateInfo->function != VK_LAYER_LINK_INFO))
        {
            layerCreateInfo = (VkLayerInstanceCreateInfo*) layerCreateInfo->pNext;
        }

        Logger::trace("vkCreateInstance");

        if (layerCreateInfo == nullptr)
        {
            // No loader instance create info
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        PFN_vkGetInstanceProcAddr gpa = layerCreateInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr;
        // move chain on for next layer
        layerCreateInfo->u.pLayerInfo = layerCreateInfo->u.pLayerInfo->pNext;

        PFN_vkCreateInstance createFunc = (PFN_vkCreateInstance) gpa(VK_NULL_HANDLE, "vkCreateInstance");

        VkInstanceCreateInfo modifiedCreateInfo = *pCreateInfo;
        VkApplicationInfo    appInfo;
        if (modifiedCreateInfo.pApplicationInfo)
        {
            appInfo = *(modifiedCreateInfo.pApplicationInfo);
            if (appInfo.apiVersion < VK_API_VERSION_1_1)
            {
                appInfo.apiVersion = VK_API_VERSION_1_1;
            }
        }
        else
        {
            appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pNext              = nullptr;
            appInfo.pApplicationName   = nullptr;
            appInfo.applicationVersion = 0;
            appInfo.pEngineName        = nullptr;
            appInfo.engineVersion      = 0;
            appInfo.apiVersion         = VK_API_VERSION_1_1;
        }

        modifiedCreateInfo.pApplicationInfo = &appInfo;
        VkResult ret                        = createFunc(&modifiedCreateInfo, pAllocator, pInstance);

        // fetch our own dispatch table for the functions we need, into the next layer
        InstanceDispatch dispatchTable;
        fillDispatchTableInstance(*pInstance, gpa, &dispatchTable);

        // store the table by key
        {
            auto&            state = GlobalState::Get();
            const std::scoped_lock lock{state.globalLock};
            state.instanceMap.emplace(GetKey(*pInstance),
                                      InstanceData{.dispatch = std::move(dispatchTable), .version = modifiedCreateInfo.pApplicationInfo->apiVersion});
        }

        return ret;
    }

    void VKAPI_CALL GlobalState::DestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator)
    {
        if (!instance)
        {
            Logger::err("null instance");
            return;
        }

        auto&            state = GlobalState::Get();
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
        auto&            state = GlobalState::Get();
        const std::scoped_lock lock{state.globalLock};
        Logger::trace("vkCreateDevice");
        VkLayerDeviceCreateInfo* layerCreateInfo = (VkLayerDeviceCreateInfo*) pCreateInfo->pNext;

        // step through the chain of pNext until we get to the link info
        while (layerCreateInfo
               && (layerCreateInfo->sType != VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO || layerCreateInfo->function != VK_LAYER_LINK_INFO))
        {
            layerCreateInfo = (VkLayerDeviceCreateInfo*) layerCreateInfo->pNext;
        }

        if (layerCreateInfo == nullptr)
        {
            // No loader instance create info
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        PFN_vkGetInstanceProcAddr gipa = layerCreateInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr;
        PFN_vkGetDeviceProcAddr   gdpa = layerCreateInfo->u.pLayerInfo->pfnNextGetDeviceProcAddr;
        // move chain on for next layer
        layerCreateInfo->u.pLayerInfo = layerCreateInfo->u.pLayerInfo->pNext;

        PFN_vkCreateDevice createFunc = (PFN_vkCreateDevice) gipa(VK_NULL_HANDLE, "vkCreateDevice");

        // check and activate extentions
        uint32_t extensionCount = 0;

        const auto instanceIt{state.instanceMap.find(GetKey(physicalDevice))};
        if (instanceIt == std::cend(state.instanceMap))
        {
            // No Instance Dispatch for Physical Device
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        const auto& [instanceKey, instanceData] = *instanceIt;

        instanceData.dispatch.EnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensionProperties(extensionCount);
        instanceData.dispatch.EnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, std::data(extensionProperties));

        bool supportsMutableFormat = false;
        for (VkExtensionProperties properties : extensionProperties)
        {
            if (properties.extensionName == "VK_KHR_swapchain_mutable_format"sv)
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
        if (modifiedCreateInfo.enabledExtensionCount)
        {
            enabledExtensionNames = std::vector<const char*>(modifiedCreateInfo.ppEnabledExtensionNames,
                                                             modifiedCreateInfo.ppEnabledExtensionNames + modifiedCreateInfo.enabledExtensionCount);
        }

        if (supportsMutableFormat)
        {
            Logger::debug("activating mutable_format");
            addUniqueCString(enabledExtensionNames, "VK_KHR_swapchain_mutable_format");
        }
        if (deviceProps.apiVersion < VK_API_VERSION_1_2 || instanceData.version < VK_API_VERSION_1_2)
        {
            addUniqueCString(enabledExtensionNames, "VK_KHR_image_format_list");
        }
        modifiedCreateInfo.ppEnabledExtensionNames = enabledExtensionNames.data();
        modifiedCreateInfo.enabledExtensionCount   = enabledExtensionNames.size();

        // Active needed Features
        VkPhysicalDeviceFeatures deviceFeatures = {};
        if (modifiedCreateInfo.pEnabledFeatures)
        {
            deviceFeatures = *(modifiedCreateInfo.pEnabledFeatures);
        }
        deviceFeatures.shaderImageGatherExtended = VK_TRUE;
        modifiedCreateInfo.pEnabledFeatures      = &deviceFeatures;

        VkResult ret = createFunc(physicalDevice, &modifiedCreateInfo, pAllocator, pDevice);

        if (ret != VkResult::VK_SUCCESS)
        {
            return ret;
        }

        LogicalDevice logicalDevice{
            .vki                   = instanceData.dispatch,
            .device                = *pDevice,
            .physicalDevice        = physicalDevice,
            .instance              = static_cast<VkInstance>(instanceKey),
            .queue                 = VK_NULL_HANDLE,
            .queueFamilyIndex      = 0,
            .commandPool           = VK_NULL_HANDLE,
            .supportsMutableFormat = supportsMutableFormat,
        };

        fillDispatchTableDevice(*pDevice, gdpa, &logicalDevice.vkd);

        uint32_t count;

        logicalDevice.vki.GetPhysicalDeviceQueueFamilyProperties(logicalDevice.physicalDevice, &count, nullptr);

        std::vector<VkQueueFamilyProperties> queueProperties(count);

        logicalDevice.vki.GetPhysicalDeviceQueueFamilyProperties(logicalDevice.physicalDevice, &count, queueProperties.data());
        for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; i++)
        {
            auto& queueInfo = pCreateInfo->pQueueCreateInfos[i];
            if ((queueProperties[queueInfo.queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT))
            {
                logicalDevice.vkd.GetDeviceQueue(logicalDevice.device, queueInfo.queueFamilyIndex, 0, &logicalDevice.queue);

                VkCommandPoolCreateInfo commandPoolCreateInfo;
                commandPoolCreateInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
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

        if (!logicalDevice.queue)
        {
            Logger::err("Did not find a graphics queue!");
        }

        state.deviceMap.emplace(GetKey(*pDevice), std::move(logicalDevice));

        return VK_SUCCESS;
    }

    void VKAPI_CALL GlobalState::DestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator)
    {
        if (!device)
        {
            Logger::err("null device");
            return;
        }

        auto&            state = GlobalState::Get();
        const std::scoped_lock lock{state.globalLock};

        Logger::trace("vkDestroyDevice");

        // TODO: extract node instead
        const auto deviceIt{state.deviceMap.find(GetKey(device))};
        if (deviceIt == std::cend(state.deviceMap))
        {
            Logger::err("could not find device in map");
            return;
        }

        auto& [_, logicalDevice] = *deviceIt;
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
        auto&            state = GlobalState::Get();
        const std::scoped_lock lock{state.globalLock};

        Logger::trace("vkCreateSwapchainKHR");

        const auto deviceIt{state.deviceMap.find(GetKey(device))};
        if (deviceIt == std::cend(state.deviceMap))
        {
            return VK_ERROR_UNKNOWN;
        }
        auto& [_, logicalDevice] = *deviceIt;

        VkSwapchainCreateInfoKHR modifiedCreateInfo = *pCreateInfo;

        VkFormat format = modifiedCreateInfo.imageFormat;

        VkFormat srgbFormat  = isSRGB(format) ? format : convertToSRGB(format);
        VkFormat unormFormat = isSRGB(format) ? convertToUNORM(format) : format;
        Logger::debug(std::to_string(srgbFormat) + " " + std::to_string(unormFormat));

        VkFormat formats[] = {unormFormat, srgbFormat};

        VkImageFormatListCreateInfoKHR imageFormatListCreateInfo;
        if (logicalDevice.supportsMutableFormat)
        {
            modifiedCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                                            | VK_IMAGE_USAGE_SAMPLED_BIT; // we want to use the swapchain images as output of the graphics pipeline
            modifiedCreateInfo.flags |= VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR;
            // TODO what if the application already uses multiple formats for the swapchain?

            imageFormatListCreateInfo.sType           = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR;
            imageFormatListCreateInfo.pNext           = modifiedCreateInfo.pNext;
            imageFormatListCreateInfo.viewFormatCount = (srgbFormat == unormFormat) ? 1 : 2;
            imageFormatListCreateInfo.pViewFormats    = formats;

            modifiedCreateInfo.pNext = &imageFormatListCreateInfo;
        }

        modifiedCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        Logger::debug("format " + std::to_string(modifiedCreateInfo.imageFormat));

        // TODO: handle make success
        auto pLogicalSwapchain = std::make_shared<LogicalSwapchain>(LogicalSwapchain{
            .pLogicalDevice      = std::addressof(logicalDevice),
            .swapchainCreateInfo = *pCreateInfo,
            .imageExtent         = modifiedCreateInfo.imageExtent,
            .format              = modifiedCreateInfo.imageFormat,
            .imageCount          = 0,
        });

        VkResult result = logicalDevice.vkd.CreateSwapchainKHR(device, &modifiedCreateInfo, pAllocator, pSwapchain);

        // TODO: check success on emplace
        state.swapchainMap.emplace(*pSwapchain, std::move(pLogicalSwapchain));

        return result;
    }

    VKAPI_ATTR VkResult VKAPI_CALL GlobalState::GetSwapchainImagesKHR(VkDevice       device,
                                                                      VkSwapchainKHR swapchain,
                                                                      uint32_t*      pCount,
                                                                      VkImage*       pSwapchainImages)
    {
        auto&            state = GlobalState::Get();
        const std::scoped_lock lock{state.globalLock};
        Logger::trace("vkGetSwapchainImagesKHR " + std::to_string(*pCount));

        const auto deviceIt{state.deviceMap.find(GetKey(device))};
        if (deviceIt == std::cend(state.deviceMap))
        {
            return VK_ERROR_UNKNOWN;
        }
        auto& [_, logicalDevice] = *deviceIt;

        if (pSwapchainImages == nullptr)
        {
            return logicalDevice.vkd.GetSwapchainImagesKHR(device, swapchain, pCount, pSwapchainImages);
        }

        LogicalSwapchain* pLogicalSwapchain = state.swapchainMap[swapchain].get();

        // If the images got already requested once, return them again instead of creating new images
        if (pLogicalSwapchain->fakeImages.size())
        {
            *pCount = std::min<uint32_t>(*pCount, pLogicalSwapchain->imageCount);
            std::memcpy(pSwapchainImages, pLogicalSwapchain->fakeImages.data(), sizeof(VkImage) * (*pCount));
            return *pCount < pLogicalSwapchain->imageCount ? VK_INCOMPLETE : VK_SUCCESS;
        }

        logicalDevice.vkd.GetSwapchainImagesKHR(device, swapchain, &pLogicalSwapchain->imageCount, nullptr);
        pLogicalSwapchain->images.resize(pLogicalSwapchain->imageCount);
        logicalDevice.vkd.GetSwapchainImagesKHR(device, swapchain, &pLogicalSwapchain->imageCount, pLogicalSwapchain->images.data());

        std::vector<std::string> effectStrings = state.config.getOption<std::vector<std::string>>("effects", {"cas"});

        // create 1 more set of images when we can't use the swapchain it self
        uint32_t fakeImageCount = pLogicalSwapchain->imageCount * (effectStrings.size() + !logicalDevice.supportsMutableFormat);

        pLogicalSwapchain->fakeImages = createFakeSwapchainImages(
            std::addressof(logicalDevice), pLogicalSwapchain->swapchainCreateInfo, fakeImageCount, pLogicalSwapchain->fakeImageMemory);
        Logger::debug("created fake swapchain images");

        VkFormat unormFormat = convertToUNORM(pLogicalSwapchain->format);
        VkFormat srgbFormat  = convertToSRGB(pLogicalSwapchain->format);

        for (uint32_t i = 0; i < effectStrings.size(); i++)
        {
            Logger::debug("current effectString " + effectStrings[i]);
            std::vector<VkImage> firstImages(pLogicalSwapchain->fakeImages.begin() + pLogicalSwapchain->imageCount * i,
                                             pLogicalSwapchain->fakeImages.begin() + pLogicalSwapchain->imageCount * (i + 1));
            Logger::debug(std::to_string(firstImages.size()) + " images in firstImages");
            std::vector<VkImage> secondImages;
            if (i == effectStrings.size() - 1)
            {
                secondImages = logicalDevice.supportsMutableFormat
                                   ? pLogicalSwapchain->images
                                   : std::vector<VkImage>(pLogicalSwapchain->fakeImages.end() - pLogicalSwapchain->imageCount,
                                                          pLogicalSwapchain->fakeImages.end());
                Logger::debug("using swapchain images as second images");
            }
            else
            {
                secondImages = std::vector<VkImage>(pLogicalSwapchain->fakeImages.begin() + pLogicalSwapchain->imageCount * (i + 1),
                                                    pLogicalSwapchain->fakeImages.begin() + pLogicalSwapchain->imageCount * (i + 2));
                Logger::debug("not using swapchain images as second images");
            }
            Logger::debug(std::to_string(secondImages.size()) + " images in secondImages");
            if (effectStrings[i] == "fxaa")
            {
                pLogicalSwapchain->effects.emplace_back(std::make_shared<FxaaEffect>(std::addressof(logicalDevice),
                                                                                     srgbFormat,
                                                                                     pLogicalSwapchain->imageExtent,
                                                                                     firstImages,
                                                                                     secondImages,
                                                                                     std::addressof(state.config)));
                Logger::debug("created FxaaEffect");
            }
            else if (effectStrings[i] == "cas")
            {
                pLogicalSwapchain->effects.emplace_back(std::make_shared<CasEffect>(std::addressof(logicalDevice),
                                                                                    unormFormat,
                                                                                    pLogicalSwapchain->imageExtent,
                                                                                    firstImages,
                                                                                    secondImages,
                                                                                    std::addressof(state.config)));
                Logger::debug("created CasEffect");
            }
            else if (effectStrings[i] == "deband")
            {
                pLogicalSwapchain->effects.emplace_back(std::make_shared<DebandEffect>(std::addressof(logicalDevice),
                                                                                       unormFormat,
                                                                                       pLogicalSwapchain->imageExtent,
                                                                                       firstImages,
                                                                                       secondImages,
                                                                                       std::addressof(state.config)));
                Logger::debug("created DebandEffect");
            }
            else if (effectStrings[i] == "smaa")
            {
                pLogicalSwapchain->effects.emplace_back(std::make_shared<SmaaEffect>(std::addressof(logicalDevice),
                                                                                     unormFormat,
                                                                                     pLogicalSwapchain->imageExtent,
                                                                                     firstImages,
                                                                                     secondImages,
                                                                                     std::addressof(state.config)));
                Logger::debug("created SmaaEffect");
            }
            else if (effectStrings[i] == "lut")
            {
                pLogicalSwapchain->effects.emplace_back(std::make_shared<LutEffect>(std::addressof(logicalDevice),
                                                                                    unormFormat,
                                                                                    pLogicalSwapchain->imageExtent,
                                                                                    firstImages,
                                                                                    secondImages,
                                                                                    std::addressof(state.config)));
                Logger::debug("created LutEffect");
            }
            else if (effectStrings[i] == "dls")
            {
                pLogicalSwapchain->effects.emplace_back(std::make_shared<DlsEffect>(std::addressof(logicalDevice),
                                                                                    unormFormat,
                                                                                    pLogicalSwapchain->imageExtent,
                                                                                    firstImages,
                                                                                    secondImages,
                                                                                    std::addressof(state.config)));
                Logger::debug("created DlsEffect");
            }
            else
            {
                pLogicalSwapchain->effects.emplace_back(std::make_shared<ReshadeEffect>(std::addressof(logicalDevice),
                                                                                        pLogicalSwapchain->format,
                                                                                        pLogicalSwapchain->imageExtent,
                                                                                        firstImages,
                                                                                        secondImages,
                                                                                        std::addressof(state.config),
                                                                                        effectStrings[i]));
                Logger::debug("created ReshadeEffect");
            }
        }

        if (!logicalDevice.supportsMutableFormat)
        {
            pLogicalSwapchain->effects.emplace_back(std::make_shared<TransferEffect>(
                std::addressof(logicalDevice),
                pLogicalSwapchain->format,
                pLogicalSwapchain->imageExtent,
                std::vector<VkImage>(pLogicalSwapchain->fakeImages.end() - pLogicalSwapchain->imageCount, pLogicalSwapchain->fakeImages.end()),
                pLogicalSwapchain->images,
                std::addressof(state.config)));
        }

        VkImageView depthImageView = logicalDevice.depthImageViews.size() ? logicalDevice.depthImageViews[0] : VK_NULL_HANDLE;
        VkImage     depthImage     = logicalDevice.depthImageViews.size() ? logicalDevice.depthImages[0] : VK_NULL_HANDLE;
        VkFormat    depthFormat    = logicalDevice.depthImageViews.size() ? logicalDevice.depthFormats[0] : VK_FORMAT_UNDEFINED;

        Logger::debug("effect string count: " + std::to_string(effectStrings.size()));
        Logger::debug("effect count: " + std::to_string(pLogicalSwapchain->effects.size()));

        pLogicalSwapchain->commandBuffersEffect = allocateCommandBuffer(std::addressof(logicalDevice), pLogicalSwapchain->imageCount);
        Logger::debug("allocated ComandBuffers " + std::to_string(pLogicalSwapchain->commandBuffersEffect.size()) + " for swapchain "
                      + convertToString(swapchain));

        writeCommandBuffers(std::addressof(logicalDevice),
                            pLogicalSwapchain->effects,
                            depthImage,
                            depthImageView,
                            depthFormat,
                            pLogicalSwapchain->commandBuffersEffect);
        Logger::debug("wrote CommandBuffers");

        pLogicalSwapchain->semaphores = createSemaphores(std::addressof(logicalDevice), pLogicalSwapchain->imageCount);
        Logger::debug("created semaphores");
        for (unsigned int i = 0; i < pLogicalSwapchain->imageCount; i++)
        {
            Logger::debug(std::to_string(i) + " written commandbuffer " + convertToString(pLogicalSwapchain->commandBuffersEffect[i]));
        }
        Logger::trace("vkGetSwapchainImagesKHR");

        pLogicalSwapchain->defaultTransfer = std::make_shared<TransferEffect>(
            std::addressof(logicalDevice),
            pLogicalSwapchain->format,
            pLogicalSwapchain->imageExtent,
            std::vector<VkImage>(pLogicalSwapchain->fakeImages.begin(), pLogicalSwapchain->fakeImages.begin() + pLogicalSwapchain->imageCount),
            pLogicalSwapchain->images,
            std::addressof(state.config));

        pLogicalSwapchain->commandBuffersNoEffect = allocateCommandBuffer(std::addressof(logicalDevice), pLogicalSwapchain->imageCount);

        writeCommandBuffers(std::addressof(logicalDevice),
                            {pLogicalSwapchain->defaultTransfer},
                            VK_NULL_HANDLE,
                            VK_NULL_HANDLE,
                            VK_FORMAT_UNDEFINED,
                            pLogicalSwapchain->commandBuffersNoEffect);

        for (unsigned int i = 0; i < pLogicalSwapchain->imageCount; i++)
        {
            Logger::debug(std::to_string(i) + " written commandbuffer " + convertToString(pLogicalSwapchain->commandBuffersNoEffect[i]));
        }

        *pCount = std::min<uint32_t>(*pCount, pLogicalSwapchain->imageCount);
        std::memcpy(pSwapchainImages, pLogicalSwapchain->fakeImages.data(), sizeof(VkImage) * (*pCount));
        return *pCount < pLogicalSwapchain->imageCount ? VK_INCOMPLETE : VK_SUCCESS;
    }

    VKAPI_ATTR VkResult VKAPI_CALL GlobalState::QueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo)
    {
        auto&            state = GlobalState::Get();
        const std::scoped_lock lock{state.globalLock};

        static uint32_t keySymbol = convertToKeySym(state.config.getOption<std::string>("toggleKey", "Home"));

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
        auto& [_, logicalDevice] = *deviceIt;

        std::vector<VkSemaphore> presentSemaphores;
        presentSemaphores.reserve(pPresentInfo->swapchainCount);

        std::vector<VkPipelineStageFlags> waitStages(pPresentInfo->waitSemaphoreCount, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        for (unsigned int i = 0; i < (*pPresentInfo).swapchainCount; i++)
        {
            uint32_t          index             = (*pPresentInfo).pImageIndices[i];
            VkSwapchainKHR    swapchain         = (*pPresentInfo).pSwapchains[i];
            LogicalSwapchain* pLogicalSwapchain = state.swapchainMap[swapchain].get(); // TODO: safe access to map

            for (auto& effect : pLogicalSwapchain->effects)
            {
                effect->updateEffect();
            }

            VkSubmitInfo submitInfo;
            submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.pNext              = nullptr;
            submitInfo.waitSemaphoreCount = i == 0 ? pPresentInfo->waitSemaphoreCount : 0;
            submitInfo.pWaitSemaphores    = i == 0 ? pPresentInfo->pWaitSemaphores : nullptr;
            submitInfo.pWaitDstStageMask  = i == 0 ? waitStages.data() : nullptr;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers =
                presentEffect ? &(pLogicalSwapchain->commandBuffersEffect[index]) : &(pLogicalSwapchain->commandBuffersNoEffect[index]);
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores    = &(pLogicalSwapchain->semaphores[index]);

            presentSemaphores.push_back(pLogicalSwapchain->semaphores[index]);

            VkResult vr = logicalDevice.vkd.QueueSubmit(logicalDevice.queue, 1, &submitInfo, VK_NULL_HANDLE);

            if (vr != VK_SUCCESS)
            {
                return vr;
            }
        }

        VkPresentInfoKHR presentInfo   = *pPresentInfo;
        presentInfo.waitSemaphoreCount = presentSemaphores.size();
        presentInfo.pWaitSemaphores    = presentSemaphores.data();

        return logicalDevice.vkd.QueuePresentKHR(queue, &presentInfo);
    }

    VKAPI_ATTR void VKAPI_CALL GlobalState::DestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator)
    {
        if (!swapchain)
        {
            Logger::err("null swapchain");
            return;
        }

        auto& state = GlobalState::Get();

        const std::scoped_lock lock{state.globalLock};
        // we need to delete the infos of the oldswapchain

        Logger::trace("vkDestroySwapchainKHR " + convertToString(swapchain));
        state.swapchainMap[swapchain]->destroy(); // TODO: safe access to map, maybe extract
        state.swapchainMap.erase(swapchain);

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
        auto&            state = GlobalState::Get();
        const std::scoped_lock lock{state.globalLock};

        const auto deviceIt{state.deviceMap.find(GetKey(device))};
        if (deviceIt == std::cend(state.deviceMap))
        {
            return VK_ERROR_UNKNOWN;
        }
        auto& [_, logicalDevice] = *deviceIt;

        if (isDepthFormat(pCreateInfo->format) && pCreateInfo->samples == VK_SAMPLE_COUNT_1_BIT
            && ((pCreateInfo->usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT))
        {
            Logger::debug("detected depth image with format: " + convertToString(pCreateInfo->format));
            Logger::debug(std::to_string(pCreateInfo->extent.width) + "x" + std::to_string(pCreateInfo->extent.height));
            Logger::debug(
                std::to_string((pCreateInfo->usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT));

            VkImageCreateInfo modifiedCreateInfo = *pCreateInfo;
            modifiedCreateInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
            VkResult result = logicalDevice.vkd.CreateImage(device, &modifiedCreateInfo, pAllocator, pImage);
            logicalDevice.depthImages.push_back(*pImage);
            logicalDevice.depthFormats.push_back(pCreateInfo->format);

            return result;
        }
        else
        {
            return logicalDevice.vkd.CreateImage(device, pCreateInfo, pAllocator, pImage);
        }
    }

    VKAPI_ATTR VkResult VKAPI_CALL GlobalState::BindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset)
    {
        auto&                  state = GlobalState::Get();
        const std::scoped_lock lock{state.globalLock};

        const auto deviceIt{state.deviceMap.find(GetKey(device))};
        if (deviceIt == std::cend(state.deviceMap))
        {
            return VK_ERROR_UNKNOWN;
        }
        auto& [_, logicalDevice] = *deviceIt;

        VkResult result = logicalDevice.vkd.BindImageMemory(device, image, memory, memoryOffset);
        // TODO what if the application creates more than one image before binding memory?
        if (logicalDevice.depthImages.size() && image == logicalDevice.depthImages.back())
        {
            Logger::debug("before creating depth image view");
            VkImageView depthImageView = createImageViews(std::addressof(logicalDevice),
                                                          logicalDevice.depthFormats[logicalDevice.depthImages.size() - 1],
                                                          {image},
                                                          VK_IMAGE_VIEW_TYPE_2D,
                                                          VK_IMAGE_ASPECT_DEPTH_BIT)[0];

            VkFormat depthFormat = logicalDevice.depthFormats[logicalDevice.depthImages.size() - 1];

            Logger::debug("created depth image view");
            logicalDevice.depthImageViews.push_back(depthImageView);
            if (logicalDevice.depthImageViews.size() > 1)
            {
                return result;
            }

            for (auto& it : state.swapchainMap)
            {
                LogicalSwapchain* pLogicalSwapchain = it.second.get();
                if (pLogicalSwapchain->pLogicalDevice == std::addressof(logicalDevice))
                {
                    if (pLogicalSwapchain->commandBuffersEffect.size())
                    {
                        logicalDevice.vkd.FreeCommandBuffers(logicalDevice.device,
                                                             logicalDevice.commandPool,
                                                             pLogicalSwapchain->commandBuffersEffect.size(),
                                                             pLogicalSwapchain->commandBuffersEffect.data());
                        pLogicalSwapchain->commandBuffersEffect.clear();
                        pLogicalSwapchain->commandBuffersEffect = allocateCommandBuffer(std::addressof(logicalDevice), pLogicalSwapchain->imageCount);
                        Logger::debug("allocated CommandBuffers for swapchain " + convertToString(it.first));

                        writeCommandBuffers(std::addressof(logicalDevice),
                                            pLogicalSwapchain->effects,
                                            image,
                                            depthImageView,
                                            depthFormat,
                                            pLogicalSwapchain->commandBuffersEffect);
                        Logger::debug("wrote CommandBuffers");
                    }
                }
            }
        }
        return result;
    }

    VKAPI_ATTR void VKAPI_CALL GlobalState::DestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator)
    {
        if (!image)
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

        auto& [_, logicalDevice] = *deviceIt;

        for (uint32_t i = 0; i < logicalDevice.depthImages.size(); i++)
        {
            if (logicalDevice.depthImages[i] == image)
            {
                logicalDevice.depthImages.erase(logicalDevice.depthImages.begin() + i);
                // TODO what if a image gets destroyed before binding memory?
                if (logicalDevice.depthImageViews.size() - 1 >= i)
                {
                    logicalDevice.vkd.DestroyImageView(logicalDevice.device, logicalDevice.depthImageViews[i], nullptr);
                    logicalDevice.depthImageViews.erase(logicalDevice.depthImageViews.begin() + i);
                }
                logicalDevice.depthFormats.erase(logicalDevice.depthFormats.begin() + i);

                VkImageView depthImageView = logicalDevice.depthImageViews.size() ? logicalDevice.depthImageViews[0] : VK_NULL_HANDLE;
                VkImage     depthImage     = logicalDevice.depthImageViews.size() ? logicalDevice.depthImages[0] : VK_NULL_HANDLE;
                VkFormat    depthFormat    = logicalDevice.depthImageViews.size() ? logicalDevice.depthFormats[0] : VK_FORMAT_UNDEFINED;
                for (auto& it : state.swapchainMap)
                {
                    LogicalSwapchain* pLogicalSwapchain = it.second.get();
                    if (pLogicalSwapchain->pLogicalDevice == std::addressof(logicalDevice))
                    {
                        if (pLogicalSwapchain->commandBuffersEffect.size())
                        {
                            logicalDevice.vkd.FreeCommandBuffers(logicalDevice.device,
                                                                 logicalDevice.commandPool,
                                                                 pLogicalSwapchain->commandBuffersEffect.size(),
                                                                 pLogicalSwapchain->commandBuffersEffect.data());
                            pLogicalSwapchain->commandBuffersEffect.clear();
                            pLogicalSwapchain->commandBuffersEffect =
                                allocateCommandBuffer(std::addressof(logicalDevice), pLogicalSwapchain->imageCount);
                            Logger::debug("allocated CommandBuffers for swapchain " + convertToString(it.first));

                            writeCommandBuffers(std::addressof(logicalDevice),
                                                pLogicalSwapchain->effects,
                                                depthImage,
                                                depthImageView,
                                                depthFormat,
                                                pLogicalSwapchain->commandBuffersEffect);
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
        if (pPropertyCount)
        {
            *pPropertyCount = 1;
        }

        // TODO: strange logic
        if (pProperties)
        {
            static constexpr auto description{"a post processing layer"sv};
            static_assert(std::size(description) < VK_MAX_DESCRIPTION_SIZE);
            std::strncpy(pProperties->description, std::data(description), std::size(description));

            static_assert(std::size(vkBasaltVkLayerName) < VK_MAX_EXTENSION_NAME_SIZE);
            std::strncpy(pProperties->layerName, std::data(vkBasaltVkLayerName), std::size(vkBasaltVkLayerName));

            pProperties->implementationVersion = 1;
            pProperties->specVersion           = VK_MAKE_VERSION(1, 2, 0);
        }

        return VK_SUCCESS;
    }

    VkResult VKAPI_CALL GlobalState::EnumerateDeviceLayerProperties(VkPhysicalDevice   physicalDevice,
                                                                    uint32_t*          pPropertyCount,
                                                                    VkLayerProperties* pProperties)
    {
        return GlobalState::EnumerateInstanceLayerProperties(pPropertyCount, pProperties);
    }

    VkResult VKAPI_CALL GlobalState::EnumerateInstanceExtensionProperties(const char*            pLayerName,
                                                                          uint32_t*              pPropertyCount,
                                                                          VkExtensionProperties* pProperties)
    {
        if (pLayerName == NULL || pLayerName == vkBasaltVkLayerName)
        {
            return VK_ERROR_LAYER_NOT_PRESENT;
        }

        // don't expose any extensions
        if (pPropertyCount)
        {
            *pPropertyCount = 0;
        }
        return VK_SUCCESS;
    }

    VkResult VKAPI_CALL GlobalState::EnumerateDeviceExtensionProperties(VkPhysicalDevice       physicalDevice,
                                                                        const char*            pLayerName,
                                                                        uint32_t*              pPropertyCount,
                                                                        VkExtensionProperties* pProperties)
    {
        // pass through any queries that aren't to us
        if (pLayerName == NULL || pLayerName == vkBasaltVkLayerName)
        {
            if (physicalDevice == VK_NULL_HANDLE)
            {
                return VK_SUCCESS;
            }

            auto&                  state = GlobalState::Get();
            const std::scoped_lock lock{state.globalLock};
            if (const auto instanceIt{state.instanceMap.find(GetKey(physicalDevice))}; instanceIt != std::cend(state.instanceMap))
            {
                return instanceIt->second.dispatch.EnumerateDeviceExtensionProperties(physicalDevice, pLayerName, pPropertyCount, pProperties);
            }

            return VK_ERROR_UNKNOWN;
        }

        // don't expose any extensions
        if (pPropertyCount)
        {
            *pPropertyCount = 0;
        }
        return VK_SUCCESS;
    }

    PFN_vkVoidFunction VKAPI_CALL GlobalState::GetDeviceProcAddr(VkDevice device, const char* pName)
    {
        // return overriden procedures
        return InterceptedCalls(pName)
            .or_else([device, pName]() -> std::optional<PFN_vkVoidFunction> {
                // return proc from device dispatch table
                auto&                  state = GlobalState::Get();
                const std::scoped_lock _{state.globalLock};
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
                const std::scoped_lock _(state.globalLock);
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
            return (PFN_vkVoidFunction) std::addressof(GetInstanceProcAddr);
        }
        if (procName == "vkEnumerateInstanceLayerProperties")
        {
            return (PFN_vkVoidFunction) std::addressof(EnumerateInstanceLayerProperties);
        }
        if (procName == "vkEnumerateInstanceExtensionProperties")
        {
            return (PFN_vkVoidFunction) std::addressof(EnumerateInstanceExtensionProperties);
        }
        if (procName == "vkCreateInstance")
        {
            return (PFN_vkVoidFunction) std::addressof(CreateInstance);
        }
        if (procName == "vkDestroyInstance")
        {
            return (PFN_vkVoidFunction) std::addressof(DestroyInstance);
        }
        // device chain functions we intercept
        // vkGetDeviceProcAddr needs to behave like vkGetInstanceProcAddr thanks to some games
        if (procName == "vkGetDeviceProcAddr")
        {
            return (PFN_vkVoidFunction) std::addressof(GetDeviceProcAddr);
        }
        if (procName == "vkEnumerateDeviceLayerProperties")
        {
            return (PFN_vkVoidFunction) std::addressof(EnumerateDeviceLayerProperties);
        }
        if (procName == "vkEnumerateDeviceExtensionProperties")
        {
            return (PFN_vkVoidFunction) std::addressof(EnumerateDeviceExtensionProperties);
        }
        if (procName == "vkCreateDevice")
        {
            return (PFN_vkVoidFunction) std::addressof(CreateDevice);
        }
        if (procName == "vkDestroyDevice")
        {
            return (PFN_vkVoidFunction) std::addressof(DestroyDevice);
        }
        if (procName == "vkCreateSwapchainKHR")
        {
            return (PFN_vkVoidFunction) std::addressof(CreateSwapchainKHR);
        }
        if (procName == "vkGetSwapchainImagesKHR")
        {
            return (PFN_vkVoidFunction) std::addressof(GetSwapchainImagesKHR);
        }
        if (procName == "vkQueuePresentKHR")
        {
            return (PFN_vkVoidFunction) std::addressof(QueuePresentKHR);
        }
        if (procName == "vkDestroySwapchainKHR")
        {
            return (PFN_vkVoidFunction) std::addressof(DestroySwapchainKHR);
        }

        // TODO: save state
        if (auto& state = GlobalState::Get(); state.config.getOption<std::string>("depthCapture", "off") != "on")
        {
            return std::nullopt;
        }

        if (procName == "vkCreateImage")
        {
            return (PFN_vkVoidFunction) std::addressof(CreateImage);
        }
        if (procName == "vkDestroyImage")
        {
            return (PFN_vkVoidFunction) std::addressof(DestroyImage);
        }
        if (procName == "vkBindImageMemory")
        {
            return (PFN_vkVoidFunction) std::addressof(BindImageMemory);
        }

        return std::nullopt;
    }

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
