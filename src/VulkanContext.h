#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

class VulkanContext
{
public:
    VulkanContext(bool validationEnabled);
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
    std::vector<const char *> layers() const;
    std::vector<const char *> extensions() const;
    void createInstance();
    void pickPhysicalDevice();
    void findQueueFamilies();
    void createLogicalDevice();
    void setupDebugMessenger();

    bool _validationEnabled = false;
    vk::UniqueInstance _instance;
    vk::PhysicalDevice _physicalDevice;
    vk::UniqueDevice _device;

    vk::Queue _graphicsQueue;
    uint32_t _graphicsQueueFamilyIndex = 0;
    vk::DebugUtilsMessengerEXT _debugMessenger;
    vk::detail::DispatchLoaderDynamic _dldy;
};
