#include "VulkanContext.h"
#include "debug.h"
#include "errors.h"
#include <spdlog/spdlog.h>

const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

VulkanContext::VulkanContext()
{
    createInstance();
#ifdef ENABLE_VALIDATION_LAYERS
    setupDebugMessenger();
#endif
    pickPhysicalDevice();
    findQueueFamilies();
    createLogicalDevice();
}

VulkanContext::~VulkanContext()
{
#ifdef ENABLE_VALIDATION_LAYERS
    _instance->destroyDebugUtilsMessengerEXT(_debugMessenger, nullptr, _dldy);
#endif
}

void VulkanContext::createInstance()
{
    vk::ApplicationInfo appInfo { "ChessVK", 1, "ChessVKEngine", 1, VK_API_VERSION_1_4 };
    vk::InstanceCreateInfo createInfo { {}, &appInfo };

#ifdef ENABLE_VALIDATION_LAYERS
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    const std::vector<const char *> extensions = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    auto debugCreateInfo = getDebugMessengerCreateInfo();
    createInfo.pNext = &debugCreateInfo;
#endif

    _instance = VK_CHECK(vk::createInstanceUnique(createInfo), "Failed to create Vulkan instance");
    spdlog::info("Vulkan instance created successfully");
}

void VulkanContext::pickPhysicalDevice()
{
    auto physicalDevices = VK_CHECK(_instance->enumeratePhysicalDevices(), "Failed to enumerate physical devices");
    if (physicalDevices.empty()) {
        throwError("Failed to find GPUs with Vulkan support!");
    }

    spdlog::info("Found {} physical devices with Vulkan support", physicalDevices.size());

    for (const auto &device : physicalDevices) {
        vk::PhysicalDeviceProperties props = device.getProperties();
        spdlog::info("  - " + std::string(props.deviceName));
        printDeviceDetails(device);
    }

    _physicalDevice = physicalDevices[0];
}

void VulkanContext::findQueueFamilies()
{
    std::vector<vk::QueueFamilyProperties> queueFamilies = _physicalDevice.getQueueFamilyProperties();

    bool foundGraphics = false;
    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            _graphicsQueueFamilyIndex = i;
            foundGraphics = true;
            break;
        }
    }

    if (!foundGraphics) {
        throwError("Failed to find a queue family that supports graphics!");
    }
}


void VulkanContext::createLogicalDevice()
{
    float queuePriority = 1.0f;
    vk::DeviceQueueCreateInfo queueCreateInfo { {}, _graphicsQueueFamilyIndex, 1, &queuePriority };
    vk::DeviceCreateInfo createInfo { {}, 1, &queueCreateInfo };

    _device = VK_CHECK(_physicalDevice.createDeviceUnique(createInfo), "Failed to create logical device");
    _graphicsQueue = _device->getQueue(_graphicsQueueFamilyIndex, 0);
}

#ifdef ENABLE_VALIDATION_LAYERS
void VulkanContext::setupDebugMessenger()
{
    _dldy = vk::detail::DispatchLoaderDynamic(*_instance, vkGetInstanceProcAddr);
    auto createInfo = getDebugMessengerCreateInfo();
    _debugMessenger = VK_CHECK(_instance->createDebugUtilsMessengerEXT(createInfo, nullptr, _dldy), "Failed to create debug messenger");
}
#endif
