#pragma once

#include <vulkan/vulkan.hpp>

class VulkanContext
{
public:
    VulkanContext();
    ~VulkanContext();

    vk::Instance getInstance() const
    {
        return *_instance;
    }

    vk::PhysicalDevice getPhysicalDevice() const
    {
        return _physicalDevice;
    }

    vk::Device getDevice() const
    {
        return *_device;
    }

    vk::Queue getGraphicsQueue() const
    {
        return _graphicsQueue;
    }

private:
    void createInstance();
    void pickPhysicalDevice();
    void findQueueFamilies();
    void createLogicalDevice();
#ifdef ENABLE_VALIDATION_LAYERS
    void setupDebugMessenger();
#endif

    vk::UniqueInstance _instance;
    vk::PhysicalDevice _physicalDevice;
    vk::UniqueDevice _device;

    vk::Queue _graphicsQueue;
    uint32_t _graphicsQueueFamilyIndex = 0;
#ifdef ENABLE_VALIDATION_LAYERS
    vk::DebugUtilsMessengerEXT _debugMessenger;
    vk::detail::DispatchLoaderDynamic _dldy;
#endif
};
