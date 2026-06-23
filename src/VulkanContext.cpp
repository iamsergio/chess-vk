#define VMA_IMPLEMENTATION

#include "VulkanContext.h"
#include "debug.h"
#include "errors.h"

#include <spdlog/spdlog.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <limits>

VulkanContext::VulkanContext(SDL_Window *window, bool validationEnabled)
    : _window(window)
    , _validationEnabled(validationEnabled)
{
    createInstance();
    if (_validationEnabled)
        setupDebugMessenger();
    pickPhysicalDevice();
    createLogicalDevice();
    setupSDL();
    findQueueFamilies();
    setupVMA();
    createSwapchain();
}

VulkanContext::~VulkanContext()
{
    _swapchain.reset();

    if (_surface != VK_NULL_HANDLE) {
        vk::detail::DispatchLoaderDynamic surfaceDldy(*_instance, vkGetInstanceProcAddr);
        _instance->destroySurfaceKHR(_surface, nullptr, surfaceDldy);
    }
    if (_validationEnabled)
        _instance->destroyDebugUtilsMessengerEXT(_debugMessenger, nullptr, _dldy);

    if (_allocator) {
        vmaDestroyAllocator(_allocator);
        _allocator = nullptr;
    }
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
    uint32_t instanceExtensionsCount = 0;
    const char *const *instanceExtensions = SDL_Vulkan_GetInstanceExtensions(&instanceExtensionsCount);
    if (!instanceExtensions) {
        throwError(std::string("Failed to get SDL Vulkan instance extensions: ") + SDL_GetError());
    }

    std::vector<const char *> extensions(instanceExtensions, instanceExtensions + instanceExtensionsCount);
    if (_validationEnabled) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
    }

    for (const auto &ext : extensions) {
        spdlog::info("Vulkan instance extension: {}", ext);
    }

    return extensions;
}

std::vector<const char *> VulkanContext::deviceExtensions() const
{
    return { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
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

    auto deviceExtensions = this->deviceExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

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

void VulkanContext::setupSDL()
{
    if (!_window) {
        throwError("SDL window is not initialized!");
    }

    SDL_CHECK(SDL_Vulkan_CreateSurface(_window, _instance.get(), nullptr, &_surface), "Failed to create Vulkan surface");
    if (_validationEnabled) {
        setObjectName(*_device, reinterpret_cast<uint64_t>(_surface), vk::ObjectType::eSurfaceKHR, "surface", _dldy);
    }

    _surfaceCapabilities = VK_CHECK(_physicalDevice.getSurfaceCapabilitiesKHR(_surface), "Failed to query surface capabilities");
}

void VulkanContext::createSwapchain()
{
    vk::Extent2D swapchainExtent = _surfaceCapabilities.currentExtent;
    if (swapchainExtent.width == std::numeric_limits<uint32_t>::max()) {
        int windowWidth = 0;
        int windowHeight = 0;
        SDL_GetWindowSize(_window, &windowWidth, &windowHeight);
        swapchainExtent = vk::Extent2D {
            static_cast<uint32_t>(windowWidth),
            static_cast<uint32_t>(windowHeight)
        };
    }

    const vk::Format imageFormat = vk::Format::eB8G8R8A8Srgb;

    vk::SwapchainCreateInfoKHR swapchainCI {};
    swapchainCI.surface = _surface;
    swapchainCI.minImageCount = _surfaceCapabilities.minImageCount;
    swapchainCI.imageFormat = imageFormat;
    swapchainCI.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    swapchainCI.imageExtent = swapchainExtent;
    swapchainCI.imageArrayLayers = 1;
    swapchainCI.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    swapchainCI.preTransform = _surfaceCapabilities.currentTransform;
    swapchainCI.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapchainCI.presentMode = vk::PresentModeKHR::eFifo;
    swapchainCI.clipped = VK_TRUE;

    _swapchain = VK_CHECK(_device->createSwapchainKHRUnique(swapchainCI), "Failed to create swapchain");
    if (_validationEnabled) {
        setObjectName(*_device, reinterpret_cast<uint64_t>(static_cast<VkSwapchainKHR>(_swapchain.get())), vk::ObjectType::eSwapchainKHR, "swapchain", _dldy);
    }
}
