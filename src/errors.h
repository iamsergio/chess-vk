#pragma once

#include <SDL3/SDL.h>

#include <spdlog/spdlog.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#include <stdexcept>
#include <string>

inline void throwError(const std::string &message)
{
    spdlog::error(message);
    throw std::runtime_error(message);
}

#define VK_CHECK(expr, msg)                            \
    [&]() {                                            \
        auto _vk_result = (expr);                      \
        if (_vk_result.result != vk::Result::eSuccess) \
            throwError(msg);                           \
        return std::move(_vk_result.value);            \
    }()

#define VK_CHECK2(expr, msg)      \
    [&]() {                       \
        auto _vk_result = (expr); \
        if (_vk_result != 0)      \
            throwError(msg);      \
    }()

#define SDL_CHECK(expr, msg)                                      \
    [&]() {                                                       \
        auto _sdl_result = (expr);                                \
        if (!_sdl_result) {                                       \
            throwError(std::string(msg) + ": " + SDL_GetError()); \
        }                                                         \
    }()

inline void setObjectName(vk::Device device, uint64_t objectHandle, vk::ObjectType objectType, const char *name, const vk::detail::DispatchLoaderDynamic &dldy)
{
    if (!name)
        return;

    vk::DebugUtilsObjectNameInfoEXT nameInfo {};
    nameInfo.objectType = objectType;
    nameInfo.objectHandle = objectHandle;
    nameInfo.pObjectName = name;

    ( void )device.setDebugUtilsObjectNameEXT(nameInfo, dldy);
}
