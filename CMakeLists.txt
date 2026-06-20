cmake_minimum_required(VERSION 3.25)
project(chess-vk CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED)
find_package(Vulkan REQUIRED)

option(CHESSVK_ENABLE_HARDENED "Enable -fhardened equivalent for GCC" OFF)

if (CHESSVK_ENABLE_HARDENED)
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -O2 -D_FORTIFY_SOURCE=3 -D_GLIBCXX_ASSERTIONS -fPIE -pie -Wl,-z,now -Wl,-z,relro -fstack-protector-strong -fstack-clash-protection -fcf-protection=full -ftrivial-auto-var-init=pattern"
    )
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")

add_executable(chess-vk main.cpp)

target_link_libraries(chess-vk PRIVATE SDL3::SDL3 Vulkan::Vulkan)
