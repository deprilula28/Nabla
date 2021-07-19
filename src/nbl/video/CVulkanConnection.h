#ifndef __NBL_C_VULKAN_CONNECTION_H_INCLUDED__
#define __NBL_C_VULKAN_CONNECTION_H_INCLUDED__

#include <volk.h>
#include "nbl/video/IAPIConnection.h"
#include "nbl/video/surface/CSurfaceVKWin32.h"
#include "nbl/video/CVulkanPhysicalDevice.h"

// Todo(achal): Need to merge this with passed SDebugCallback to CVulkanConnection
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData)
{
    printf("Validation Layer: %s\n", callbackData->pMessage);
    return VK_FALSE;
}

namespace nbl
{
namespace video
{

class CVulkanConnection final : public IAPIConnection
{
public:
    CVulkanConnection(uint32_t appVer, const char* appName, const SDebugCallback& dbgCb) : IAPIConnection(dbgCb)
    {
        VkResult result = volkInitialize();
        assert(result == VK_SUCCESS);
        
        VkApplicationInfo applicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        applicationInfo.apiVersion = VK_MAKE_VERSION(1, 1, 0);
        applicationInfo.engineVersion = NABLA_VERSION_INTEGER;
        applicationInfo.pEngineName = "Nabla";
        applicationInfo.applicationVersion = appVer;
        applicationInfo.pApplicationName = appName;
        
        // Todo(achal): Where to put this? Also, do we need more (validation) layers, if there are?
        const uint32_t instanceLayerCount = 1u;
        const char* instanceLayers[instanceLayerCount] = { "VK_LAYER_KHRONOS_validation" }; // Todo(achal): Need to check availability for this

        // Todo(achal): This needs to be handled in a platform agnostic way.
        // These extensions are necessary for creating Vulkan surfaces.
        // VK_KHR_surface introduces VkSurfaceKHR
        // Not sure if we want VK_KHR_win32_surface yet
        // Todo(achal): Not always use the debug messenger
        const uint32_t instanceExtensionCount = 3u;
        const char* instanceExtensions[instanceExtensionCount] = { "VK_KHR_surface", "VK_KHR_win32_surface", VK_EXT_DEBUG_UTILS_EXTENSION_NAME }; // Todo(achal): Need to check availability for this

        // Note(achal): This debug messenger is not application wide but _only_ for instance creation. In other words, it is for internal
        // use only.
        VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
        debugUtilsMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugUtilsMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugUtilsMessengerCreateInfo.pfnUserCallback = debugCallback;

        VkInstanceCreateInfo instanceCreateInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugUtilsMessengerCreateInfo;
        instanceCreateInfo.pApplicationInfo = &applicationInfo;
        instanceCreateInfo.enabledLayerCount = instanceLayerCount;
        instanceCreateInfo.ppEnabledLayerNames = instanceLayers;
        instanceCreateInfo.enabledExtensionCount = instanceExtensionCount;
        instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions;
        
        vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance);
        assert(m_instance);
        
        // Todo(achal): I gotta look into this ie if (and how) we want to use volkLoadInstanceOnly
        // volkLoadInstanceOnly(m_instance);
        volkLoadInstance(m_instance);
        
        uint32_t devCount = 0u;
        vkEnumeratePhysicalDevices(m_instance, &devCount, nullptr);
        core::vector<VkPhysicalDevice> vkphds(devCount, VK_NULL_HANDLE);
        vkEnumeratePhysicalDevices(m_instance, &devCount, vkphds.data());

        m_physDevices = core::make_refctd_dynamic_array<physical_devs_array_t>(devCount);
        for (uint32_t i = 0u; i < devCount; ++i)
        {
            (*m_physDevices)[i] = core::make_smart_refctd_ptr<CVulkanPhysicalDevice>(vkphds[i], std::move(m_fs), std::move(m_GLSLCompiler));
        }
    }

    E_API_TYPE getAPIType() const override { return EAT_VULKAN; }

    core::SRange<const core::smart_refctd_ptr<IPhysicalDevice>> getPhysicalDevices() const override
    {
        return core::SRange<const core::smart_refctd_ptr<IPhysicalDevice>>{ m_physDevices->begin(), m_physDevices->end() };
    }

    core::smart_refctd_ptr<ISurface> createSurface(ui::IWindow* window) const override
    {
        return nullptr;
    }

    VkInstance getInternalObject() const { return m_instance; }

protected:
    ~CVulkanConnection()
    {
        vkDestroyInstance(m_instance, nullptr);
    }

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    using physical_devs_array_t = core::smart_refctd_dynamic_array<core::smart_refctd_ptr<IPhysicalDevice>>;
    physical_devs_array_t m_physDevices;
};

}
}


#endif
