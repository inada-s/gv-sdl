#pragma once

#include <SDL.h>
#include <SDL2_gfxPrimitives.h>
#include <functional>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

namespace gv {
using std::string;
using std::vector;

SDL_mutex* _mutex;
vector<char> _commands;
int _num_of_time = 0;
int _vis_base_index = 0;

double _current_time;
bool _initialized = false;
SDL_Window* _window;
SDL_Renderer* _render;
vector<char> _current;

struct Color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
  Color(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 0xFF)
      : r(r), g(g), b(b), a(a) {}
};

template <typename T>
inline void _gv_write(vector<char>& buf, T& value) {
  buf.insert(buf.end(), reinterpret_cast<char*>(&value),
             reinterpret_cast<char*>(&value + 1));
}
template <typename T>
inline T* _gv_read(vector<char>& buf, int* p) {
  int i = *p;
  *p += sizeof(T);
  return reinterpret_cast<T*>(&buf[i]);
}

void _gv_init() {
  _current_time = 0;
  _num_of_time = 0;

  _current.push_back('t');
  _gv_write(_current, _current_time);

  SDL_Init(SDL_INIT_VIDEO);
  // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);
  _window = SDL_CreateWindow("Hey", SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED, 960, 640, 0);
  _render = SDL_CreateRenderer(
      _window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
}

void _gv_main_loop() {
  SDL_Event ev;
  bool running = true;

  while (running) {
    SDL_SetRenderDrawColor(_render, 0, 0, 0, 255);
    SDL_RenderClear(_render);

    while (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_QUIT) {
        running = false;
      }
    }

    int index = _vis_base_index;  // shoud lock
    bool visit_t = false;
    bool cont = true;
    double vis_time = 0;

    while (cont && index < _commands.size()) {
      auto cmd = _gv_read<char>(_commands, &index);
      switch (*cmd) {
        case 't': {
          std::cerr << "t command" << std::endl;
          if (visit_t) {
            cont = false;
          } else {
            visit_t = true;
            vis_time = *_gv_read<double>(_commands, &index);
          }
          break;
        }
        case 'l': {
          std::cerr << "l command" << std::endl;
          const auto x1 = _gv_read<double>(_commands, &index);
          const auto y1 = _gv_read<double>(_commands, &index);
          const auto x2 = _gv_read<double>(_commands, &index);
          const auto y2 = _gv_read<double>(_commands, &index);
          const auto c = _gv_read<Color>(_commands, &index);
          thickLineRGBA(_render, *x1, *y1, *x2, *y2, 2, c->r, c->g, c->b, c->a);
          break;
        }
        case 'r': {
          const auto x = _gv_read<double>(_commands, &index);
          const auto y = _gv_read<double>(_commands, &index);
          const auto w = _gv_read<double>(_commands, &index);
          const auto h = _gv_read<double>(_commands, &index);
          const auto c = _gv_read<Color>(_commands, &index);
          SDL_SetRenderDrawColor(_render, c->r, c->g, c->b, c->a);
          SDL_Rect rect;
          rect.x = *x;
          rect.y = *y;
          rect.w = *w;
          rect.h = *h;
          SDL_RenderDrawRect(_render, &rect);
          break;
        }
        case 'c': {
          std::cerr << "c command" << std::endl;
          const auto x = _gv_read<double>(_commands, &index);
          const auto y = _gv_read<double>(_commands, &index);
          const auto r = _gv_read<double>(_commands, &index);
          const auto f = _gv_read<bool>(_commands, &index);
          const auto c = _gv_read<Color>(_commands, &index);
          if (*f) {
            filledCircleRGBA(_render, *x, *y, *r, c->r, c->g, c->b, c->a);
          } else {
            circleRGBA(_render, *x, *y, *r, c->r, c->g, c->b, c->a);
          }
          break;
        }
        default: {
          std::cerr << "Unknown command" << std::endl;
          break;
        }
      }
    }

    SDL_RenderPresent(_render);
  }
  SDL_Quit();
}

void gvRunMainThread(std::function<void()> f) {
  _gv_init();
  _initialized = true;
  auto th = std::thread(f);
  th.detach();
  _gv_main_loop();
}

void gvRunSubThread() {
  _gv_init();
  _initialized = true;
  auto th = std::thread(_gv_main_loop);
  th.detach();
}

void gvLine(double x1, double y1, double x2, double y2, Color color) {
  _current.push_back('l');
  _gv_write(_current, x1);
  _gv_write(_current, y1);
  _gv_write(_current, x2);
  _gv_write(_current, y2);
  _gv_write(_current, color);
}

void gvCircle(double x, double y, double r, bool filled, Color color) {
  _current.push_back('c');
  _gv_write(_current, x);
  _gv_write(_current, y);
  _gv_write(_current, r);
  _gv_write(_current, filled);
  _gv_write(_current, color);
}

void gvRect(double x, double y, double w, double h, bool filled, Color color) {
  _current.push_back('r');
  _gv_write(_current, x);
  _gv_write(_current, y);
  _gv_write(_current, w);
  _gv_write(_current, h);
  _gv_write(_current, filled);
  _gv_write(_current, color);
}

void gvArrow(double x1, double y1, double x2, double y2, Color color) {
  const auto w = 10.0;
  const auto h = 10.0;
  const auto dx = x2 - x1;
  const auto dy = y2 - y1;
  const auto v = sqrt(dx * dx + dy * dy);
  const auto ux = dx / v;
  const auto uy = dy / v;
  const auto lx = x2 - uy * w - ux * h;
  const auto ly = y2 + ux * w - uy * h;
  const auto rx = x2 + uy * w - ux * h;
  const auto ry = y2 - ux * w - uy * h;
  const auto mx = x2 - ux * h * 0.8;
  const auto my = y2 - uy * h * 0.8;
  gvLine(x1, y1, x2, y2, color);
  gvLine(lx, ly, x2, y2, color);
  gvLine(rx, ry, x2, y2, color);
  gvLine(lx, ly, mx, my, color);
  gvLine(rx, ry, mx, my, color);
}

inline void _gv_flush_locked() {
  if (_current.empty()) {
    return;
  }
  _vis_base_index = static_cast<int>(_commands.size());
  _commands.insert(_commands.end(), std::make_move_iterator(_current.begin()),
                   std::make_move_iterator(_current.end()));
  _current.clear();
}

void gvFlush() { _gv_flush_locked(); }

void gvNewTime() {
  _gv_flush_locked();
  _current.push_back('t');
  _gv_write(_current, _current_time);
  _num_of_time++;
}
}
