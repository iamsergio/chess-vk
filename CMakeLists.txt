cmake_minimum_required(VERSION 3.25)
project(chess-vk CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(chess-vk main.cpp)

target_link_libraries(chess-vk PRIVATE SDL3::SDL3)
