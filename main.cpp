#include "src/VulkanContext.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <spdlog/spdlog.h>
#include <string_view>

#if defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define HAS_TSAN
#endif
#elif defined(__SANITIZE_THREAD__) // For GCC
#define HAS_TSAN
#endif

#ifdef HAS_TSAN
#include <sanitizer/tsan_interface.h>

extern "C" const char *__tsan_default_suppressions()
{
    return "mutex:libdbus-1.so\n"
           "called_from_lib:libdbus-1.so.3\n"
           "mutex:libSDL3.so.0\n";
}
#endif

bool contains_flag(int argc, char *argv[], std::string_view flag)
{
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) == flag)
            return true;
    }

    return false;
}

int main(int argc, char *argv[])
{
    if (contains_flag(argc, argv, "-h") || contains_flag(argc, argv, "--help")) {
        spdlog::info("Usage: chess-vk [--validation] [-h|--help]");
        spdlog::info("  --validation  Enable validation layers");
        spdlog::info("  -h, --help    Show this help message");
        return 0;
    }

    const bool validationEnabled = contains_flag(argc, argv, "--validation");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        spdlog::error("SDL_Init failed: {}", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("chess-vk", 800, 600, 0);
    if (!window) {
        spdlog::error("SDL_CreateWindow failed: {}", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    VulkanContext context(validationEnabled);

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
