#include <SDL.h>
#include <functional>
#include <thread>
#include <vector>

SDL_Window* window;
SDL_Renderer* render;

int gvMain(std::function<void()> f) {
  SDL_Init(SDL_INIT_VIDEO);
  window = SDL_CreateWindow("Hey", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, 960, 640, 0);
  render = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

  SDL_Event ev;

  auto th = std::thread(f);
  th.detach();

  while (true) {
    SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
    SDL_RenderClear(render);
    while (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_QUIT) return 0;
    }
    SDL_SetRenderDrawColor(render, 255, 0, 0, 255);
    // SDL_RenderDrawLine(render, 10, 10, 400, 400);
    SDL_RenderPresent(render);
  }

  SDL_Quit();
}

struct GV_RGB {
  int r;
  int g;
  int b;
  int toInt() const { return ((r & 255) << 16) | ((g & 255) << 8) | (b & 255); }
};

GV_RGB gvRGB(int r, int g, int b) {
  GV_RGB result;
  result.r = r;
  result.g = g;
  result.b = b;
  return result;
}

void gvLine(double x1, double y1, double x2, double y2) {
  // SDL_RenderDrawLine(render, x1, y1, x2, y2);
}