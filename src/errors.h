#pragma once

#include <spdlog/spdlog.h>

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
