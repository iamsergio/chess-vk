#pragma once

#include "errors.h"

#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

#include <string>

inline void printQueues(vk::PhysicalDevice device)
{
    auto queueFamilies = device.getQueueFamilyProperties();
    spdlog::info("  Queues:");
    for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilies.size()); ++i) {
        const auto &qf = queueFamilies[i];
        std::string flags;
        if (qf.queueFlags & vk::QueueFlagBits::eGraphics)
            flags += "Graphics ";
        if (qf.queueFlags & vk::QueueFlagBits::eCompute)
            flags += "Compute ";
        if (qf.queueFlags & vk::QueueFlagBits::eTransfer)
            flags += "Transfer ";
        spdlog::info("    Family " + std::to_string(i) + ": count=" + std::to_string(qf.queueCount) + " flags=[" + flags + "]");
    }
}

inline void printDeviceDetails(vk::PhysicalDevice device)
{
    vk::PhysicalDeviceProperties props = device.getProperties();
    spdlog::info("  Type: " + vk::to_string(props.deviceType));
    spdlog::info("  API: " + std::to_string(VK_VERSION_MAJOR(props.apiVersion)) + "."
                 + std::to_string(VK_VERSION_MINOR(props.apiVersion)) + "."
                 + std::to_string(VK_VERSION_PATCH(props.apiVersion)));

    vk::PhysicalDeviceMemoryProperties memProps = device.getMemoryProperties();
    spdlog::info("  Memory Heaps:");
    for (uint32_t i = 0; i < memProps.memoryHeapCount; ++i) {
        float sizeGB = static_cast<float>(memProps.memoryHeaps[i].size) / (1024.0f * 1024.0f * 1024.0f);
        std::string heapType = (memProps.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal)
            ? "DEVICE_LOCAL (VRAM)"
            : "HOST_VISIBLE (System RAM)";
        spdlog::info("    Heap " + std::to_string(i) + ": " + std::to_string(sizeGB) + " GB | " + heapType);
    }

    spdlog::info("  Limits:");
    spdlog::info("    Max 2D Texture: " + std::to_string(props.limits.maxImageDimension2D) + "x" + std::to_string(props.limits.maxImageDimension2D));
    spdlog::info("    Max 3D Texture: " + std::to_string(props.limits.maxImageDimension3D) + "x" + std::to_string(props.limits.maxImageDimension3D));
    spdlog::info("    Max Viewports: " + std::to_string(props.limits.maxViewports));
    spdlog::info("    Max Compute Work Group Invocations: " + std::to_string(props.limits.maxComputeWorkGroupInvocations));

    auto extensions = VK_CHECK(device.enumerateDeviceExtensionProperties(), "Failed to enumerate extensions");
    bool hasRayTracing = false;
    for (const auto &ext : extensions) {
        if (std::string(ext.extensionName) == "VK_KHR_ray_tracing_pipeline") {
            hasRayTracing = true;
            break;
        }
    }
    spdlog::info("  Ray Tracing: " + std::string(hasRayTracing ? "Yes" : "No"));

    printQueues(device);
}

#ifdef ENABLE_VALIDATION_LAYERS
static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/,
    vk::DebugUtilsMessageTypeFlagsEXT /*messageType*/,
    const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void * /*pUserData*/)
{
    spdlog::warn("Validation: {}", pCallbackData->pMessage);
    return vk::False;
}

inline vk::DebugUtilsMessengerCreateInfoEXT getDebugMessengerCreateInfo()
{
    vk::DebugUtilsMessengerCreateInfoEXT createInfo {};
    createInfo.messageSeverity =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    createInfo.messageType =
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    createInfo.pfnUserCallback = debugCallback;
    return createInfo;
}
#endif
