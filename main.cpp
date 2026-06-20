#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#if defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define HAS_TSAN
#endif
#elif defined(__SANITIZE_THREAD__) // For GCC
#define HAS_TSAN
#endif

#ifdef HAS_TSAN
#include <sanitizer/tsan_interface.h>

extern "C" const char *__tsan_default_suppressions() {
  return "mutex:libdbus-1.so\n"
         "called_from_lib:libdbus-1.so.3\n"
         "mutex:libSDL3.so.0\n";
}
#endif

int main(int argc, char *argv[]) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow("chess-vk", 800, 600, 0);
  if (!window) {
    SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

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
