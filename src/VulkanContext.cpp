#define VMA_IMPLEMENTATION

#include "VulkanContext.h"
#include "debug.h"
#include "errors.h"

#include <spdlog/spdlog.h>

VulkanContext::VulkanContext(bool validationEnabled)
    : _validationEnabled(validationEnabled)
{
    createInstance();
    if (_validationEnabled)
        setupDebugMessenger();
    pickPhysicalDevice();
    findQueueFamilies();
    createLogicalDevice();
}

VulkanContext::~VulkanContext()
{
    if (_validationEnabled)
        _instance->destroyDebugUtilsMessengerEXT(_debugMessenger, nullptr, _dldy);
}

void VulkanContext::createInstance()
{
    vk::ApplicationInfo appInfo { "ChessVK", 1, "ChessVKEngine", 1, VK_API_VERSION_1_4 };
    vk::InstanceCreateInfo createInfo { {}, &appInfo };

    const auto layersList = this->layers();
    createInfo.enabledLayerCount = static_cast<uint32_t>(layersList.size());
    createInfo.ppEnabledLayerNames = layersList.data();

    const auto extensionsList = this->extensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionsList.size());
    createInfo.ppEnabledExtensionNames = extensionsList.data();

    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (_validationEnabled) {
        debugCreateInfo = getDebugMessengerCreateInfo();
        createInfo.pNext = &debugCreateInfo;
    }

    _instance = VK_CHECK(vk::createInstanceUnique(createInfo), "Failed to create Vulkan instance");
    spdlog::info("Vulkan instance created successfully");
}

std::vector<const char *> VulkanContext::layers() const
{
    if (!_validationEnabled) {
        return {};
    }

    return { "VK_LAYER_KHRONOS_validation" };
}

std::vector<const char *> VulkanContext::extensions() const
{
    if (!_validationEnabled) {
        return {};
    }

    return { VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME };
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

void VulkanContext::setupDebugMessenger()
{
    _dldy = vk::detail::DispatchLoaderDynamic(*_instance, vkGetInstanceProcAddr);
    auto createInfo = getDebugMessengerCreateInfo();
    _debugMessenger = VK_CHECK(_instance->createDebugUtilsMessengerEXT(createInfo, nullptr, _dldy), "Failed to create debug messenger");
}

void VulkanContext::setupVMA()
{
    VmaVulkanFunctions vkFunctions {
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
        .vkCreateImage = vkCreateImage
    };

    VmaAllocatorCreateInfo allocatorCI {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = _physicalDevice,
        .device = *_device,
        .pVulkanFunctions = &vkFunctions,
        .instance = _instance.get()
    };

    VK_CHECK2(vmaCreateAllocator(&allocatorCI, &_allocator), "Failed to create VMA allocator");
}
