#pragma once

#include <GL/glu.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_ttf.h>

#include <assert.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace gv_internal {

inline unsigned next_power_of_two(unsigned v) {
  assert(v > 0);
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

struct GvColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
  GvColor(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 0xFF)
      : r(r), g(g), b(b), a(a) {}
};

template <class T>
struct Point {
  T x, y;
  Point() {}
  Point(T x, T y) : x(x), y(y) {}
};

template <class T>
struct BoundingBox {
  T lx, ly, ux, uy;
  BoundingBox() {
    lx = ly = std::numeric_limits<T>::max();
    ux = uy = std::numeric_limits<T>::min();
  }

  template <typename U>
  void Update(const U& u) {
    lx = std::min(lx, u.MinX());
    ly = std::min(ly, u.MinY());
    ux = std::max(ux, u.MaxX());
    uy = std::max(uy, u.MaxY());
  }
};

class BinaryWriter {
 public:
  BinaryWriter(std::vector<char>& buf) : buf(buf) {}
  template <typename T>
  void Write(T value) {
    buf.insert(buf.end(), reinterpret_cast<char*>(&value),
               reinterpret_cast<char*>(&value + 1));
  }

  void Write(const std::string& s) {
    const std::string::size_type n = s.size();
    Write(n);
    buf.insert(buf.end(), s.begin(), s.end());
  }

 private:
  std::vector<char>& buf;
};

class BinaryReader {
 public:
  BinaryReader(std::vector<char>& buf, size_t pos = 0) : buf(buf), pos_(pos) {}
  template <typename T>
  void Read(T& dst) {
    dst = *reinterpret_cast<T*>(&buf[pos_]);
    pos_ += sizeof(T);
  }

  void Read(std::string& s) {
    std::string::size_type n;
    Read(n);
    s = std::string(&buf[pos_], n);
    // s.assign(&buf[pos_], n);
    pos_ += n;
  }

  void pos(size_t pos) { pos_ = pos; }
  size_t pos() const { return pos_; }

 private:
  std::vector<char>& buf;
  size_t pos_;
};

template <class T>
struct RenderArgs {
  SDL_Renderer* renderer;
  TTF_Font* font;
  std::function<void(double, double, double, int, int, GvColor, const char*)>
      render_text_func;
};

template <class T>
struct GvPolygonItem {
  std::vector<T> vx, vy;
  GvColor c;

  T MinX() const { return *std::min_element(begin(vx), end(vx)); }
  T MinY() const { return *std::min_element(begin(vy), end(vy)); }
  T MaxX() const { return *std::max_element(begin(vx), end(vx)); }
  T MaxY() const { return *std::max_element(begin(vy), end(vy)); }

  template <typename Writer>
  void WriteTo(Writer& w) {
    const size_t n = vx.size();
    w.Write(n);
    for (int i = 0; i < n; ++i) w.Write(vx[i]);
    for (int i = 0; i < n; ++i) w.Write(vy[i]);
    w.Write(c);
  }

  template <typename Reader>
  static void ReadFrom(Reader& r, GvPolygonItem& dst) {
    size_t n;
    r.Read(n);
    dst.vx.resize(n);
    dst.vy.resize(n);
    for (int i = 0; i < n; ++i) r.Read(dst.vx[i]);
    for (int i = 0; i < n; ++i) r.Read(dst.vy[i]);
    r.Read(dst.c);
  }

  void Render(const RenderArgs<T>& r) const {
    const auto n = vx.size();
    glColor4f(c.r / 256.0, c.g / 256.0, c.b / 256.0, c.a / 256.0);
    glBegin(GL_POLYGON);
    for (int i = 0; i < n; ++i) {
      glVertex2d(vx[i], vy[i]);
    }
    glEnd();
  }
};

template <class T>
struct GvTextItem {
  double x, y, r;
  GvColor c;
  std::string text;

  template <typename Writer>
  void WriteTo(Writer& w) {
    w.Write(x);
    w.Write(y);
    w.Write(r);
    w.Write(c);
    w.Write(text);
  }

  template <typename Reader>
  static void ReadFrom(Reader& r, GvTextItem& dst) {
    r.Read(dst.x);
    r.Read(dst.y);
    r.Read(dst.r);
    r.Read(dst.c);
    r.Read(dst.text);
  }

  T minx = std::numeric_limits<T>::max();
  T miny = std::numeric_limits<T>::max();
  T maxx = std::numeric_limits<T>::min();
  T maxy = std::numeric_limits<T>::min();

  T MinX() const { return minx; }
  T MinY() const { return miny; }
  T MaxX() const { return maxx; }
  T MaxY() const { return maxy; }

  void Render(const RenderArgs<T>& r) {
    int w, h;
    TTF_SizeText(r.font, text.c_str(), &w, &h);
    double scale = this->r / h;
    minx = std::round(this->x - w * scale * 0.5);
    maxx = std::round(this->x + w * scale * 0.5);
    miny = std::round(this->y - h * scale * 0.5);
    maxy = std::round(this->y + h * scale * 0.5);
    r.render_text_func(x, y, this->r, 0, 0, c, text.c_str());
  }
};

template <class T>
struct GvCircleItem {
  Point<T> p;
  T r;
  GvColor c;

  T MinX() const { return p.x - r; }
  T MinY() const { return -p.y - r; }
  T MaxX() const { return p.x + r; }
  T MaxY() const { return -p.y + r; }

  void Render(const RenderArgs<T>& r) const {
    const auto n = 64;
    glColor4f(c.r / 256.0, c.g / 256.0, c.b / 256.0, c.a / 256.0);
    glBegin(GL_POLYGON);
    for (int i = 0; i < n; i++) {
      const auto rate = (double)i / n;
      const auto x = p.x + this->r * cos(2.0 * M_PI * rate);
      const auto y = p.y + this->r * sin(2.0 * M_PI * rate);
      glVertex2d(x, y);
    }
    glEnd();
  }
};

class GvSDL {
 public:
  GvColor Color(int8_t r = 0, uint8_t g = 0, uint8_t b = 0,
                uint8_t a = 0xFF) const {
    return GvColor(r, g, b, a);
  }

  GvColor ColorIndex(int index) const {
    constexpr uint32_t colors[] = {
        0x000000, 0x003300, 0x003333, 0x333333, 0x660066, 0x663300, 0x000080,
        0x008000, 0x008080, 0x800000, 0x800080, 0x808000, 0x808080, 0x969696,
        0x003399, 0x663399, 0x669933, 0x993333, 0x996666, 0xc0c0c0, 0x00cc99,
        0xcc6600, 0xcccc33, 0x0000ff, 0x0066ff, 0x0099ff, 0x00ccff, 0x00ff00,
        0x00ffff, 0x8080ff, 0x99ccff, 0x99ffff, 0xcc99ff, 0xccffcc, 0xccffff,
        0xff0000, 0xff00ff, 0xff6633, 0xff9999, 0xff99cc, 0xffcc00, 0xffcc99,
        0xffcccc, 0xffff00, 0xffffcc, 0xffffff};
    const auto value = colors[index % (sizeof(colors) / sizeof(uint32_t))];
    return GvColor((value & 0xFF0000) >> 16, (value & 0x00FF00) >> 8,
                   (value & 0x0000FF), default_alpha());
  }

  void Flush() {
    if (!enabled()) return;
    mtx.lock();
    FlushLocked();
    mtx.unlock();
  }

  void NewTime() {
    if (!enabled()) return;
    mtx.lock();
    FlushLocked();
    buffer.push_back('n');
    BinaryWriter(buffer).Write(buffer_time);
    buffer_time += 1.0;
    mtx.unlock();
  }

  void RunMainThread(std::function<void()> f) {
    if (!enabled()) {
      f();
      return;
    }
    Init();
    initialized = true;
    auto th = std::thread(f);
    th.detach();
    MainLoop();
  }

  void RunSubThread() {
    if (!enabled()) return;
    Init();
    initialized = true;
    auto th = std::thread(&GvSDL::MainLoop, this);
    th.detach();
  }

  void Line(double x1, double y1, double x2, double y2, double r,
            GvColor color) {
    if (!enabled()) return;
    constexpr double sqrt2 = 1.41421356237;
    const double odx = x2 - x1;
    const double ody = y2 - y1;
    const double rate = r / sqrt(odx * odx + ody * ody);
    const double dx = odx * rate;
    const double dy = ody * rate;
    GvPolygonItem<double> item;
    item.vx = {x2 - dy * (0.05 / (1 + sqrt2)),
               x2 - dx * (0.05 * sqrt2 / (1 + sqrt2)) - dy * 0.05,
               x1 + dx * (0.05 * sqrt2 / (1 + sqrt2)) - dy * 0.05,
               x1 - dy * (0.05 / (1 + sqrt2)),
               x1 + dy * (0.05 / (1 + sqrt2)),
               x1 + dx * (0.05 * sqrt2 / (1 + sqrt2)) + dy * 0.05,
               x2 - dx * (0.05 * sqrt2 / (1 + sqrt2)) + dy * 0.05,
               x2 + dy * (0.05 / (1 + sqrt2))};
    item.vy = {y2 + dx * (0.05 / (1 + sqrt2)),
               y2 - dy * (0.05 * sqrt2 / (1 + sqrt2)) + dx * 0.05,
               y1 + dy * (0.05 * sqrt2 / (1 + sqrt2)) + dx * 0.05,
               y1 + dx * (0.05 / (1 + sqrt2)),
               y1 - dx * (0.05 / (1 + sqrt2)),
               y1 + dy * (0.05 * sqrt2 / (1 + sqrt2)) - dx * 0.05,
               y2 - dy * (0.05 * sqrt2 / (1 + sqrt2)) - dx * 0.05,
               y2 - dx * (0.05 / (1 + sqrt2))};
    item.c = color;
    buffer.push_back('p');
    auto w = BinaryWriter(buffer);
    item.WriteTo(w);
  }

  void Circle(double x, double y, double r, bool filled, GvColor color) {
    if (!enabled()) return;
    GvCircleItem<double> item;
    item.p.x = x;
    item.p.y = y;
    item.r = r;
    item.c = color;
    buffer.push_back('c');
    auto w = BinaryWriter(buffer);
    w.Write(item);
  }

  void Rect(double x, double y, double w, double h, GvColor color) {
    if (!enabled()) return;
    GvPolygonItem<double> item;
    item.vx.push_back(x);
    item.vx.push_back(x);
    item.vx.push_back(x + w);
    item.vx.push_back(x + w);
    item.vy.push_back(y);
    item.vy.push_back(y + h);
    item.vy.push_back(y + h);
    item.vy.push_back(y);
    item.c = color;
    auto wr = BinaryWriter(buffer);
    buffer.push_back('p');
    item.WriteTo(wr);
  }

  void Text(double x, double y, double r, GvColor color,
            const char* format = "?", ...) {
    if (!enabled()) return;
    char buf[256];
    va_list arg;
    va_start(arg, format);
    auto size = vsnprintf(buf, 256, format, arg);
    va_end(arg);
    if (size < 0) return;
    GvTextItem<double> item;
    item.x = x;
    item.y = y;
    item.r = r;
    item.c = color;
    item.text.assign(buf, size);
    auto wr = BinaryWriter(buffer);
    buffer.push_back('t');
    item.WriteTo(wr);
  }

  void Arrow(double x1, double y1, double x2, double y2, double r,
             GvColor color) {
    if (!enabled()) return;
    constexpr double sqrt2 = 1.41421356237;
    constexpr double sinA = 0.2588190451;   // sin(M_PI * 15 / 180);
    constexpr double cosA = 0.96592582628;  // cos(M_PI * 15 / 180);
    constexpr double tanA = 0.26794919243;  // tan(M_PI * 15 / 180);
    const double odx = x2 - x1;
    const double ody = y2 - y1;
    const double rate = r / sqrt(odx * odx + ody * ody);
    const double dx = odx * rate;
    const double dy = ody * rate;
    const double x2_base = x2 + dx * 0.1;
    const double y2_base = y2 + dy * 0.1;
    const double dx0 = dx * 0.1 * tanA;
    const double dy0 = dy * 0.1 * tanA;
    const double x2_3 = x2_base - dx * (0.1 / sinA);
    const double y2_3 = y2_base - dy * (0.1 / sinA);
    const double x2_4 = x2_3 - dx * (0.05 / tanA);
    const double y2_4 = y2_3 - dy * (0.05 / tanA);
    const double x2_5 = x2_base - dx * (1.0 * cosA);
    const double y2_5 = y2_base - dy * (1.0 * cosA);
    const double x2_6 = x2_5 - dx * (0.1 * sinA);
    const double y2_6 = y2_5 - dy * (0.1 * sinA);
    const double dx5 = dx * (1.0 * sinA);
    const double dy5 = dy * (1.0 * sinA);
    const double dx6 = dx5 - dx * (0.1 * cosA);
    const double dy6 = dy5 - dy * (0.1 * cosA);
    GvPolygonItem<double> item;
    item.vx = {x2 - dy0,
               x2_5 - dy5,
               x2_6 - dy6,
               x2_4 - dy * 0.05,
               x1 + dx * (0.05 * sqrt2 / (1 + sqrt2)) - dy * 0.05,
               x1 - dy * (0.05 / (1 + sqrt2)),
               x1 + dy * (0.05 / (1 + sqrt2)),
               x1 + dx * (0.05 * sqrt2 / (1 + sqrt2)) + dy * 0.05,
               x2_4 + dy * 0.05,
               x2_6 + dy6,
               x2_5 + dy5,
               x2 + dy0};
    item.vy = {y2 + dx0,
               y2_5 + dx5,
               y2_6 + dx6,
               y2_4 + dx * 0.05,
               y1 + dy * (0.05 * sqrt2 / (1 + sqrt2)) + dx * 0.05,
               y1 + dx * (0.05 / (1 + sqrt2)),
               y1 - dx * (0.05 / (1 + sqrt2)),
               y1 + dy * (0.05 * sqrt2 / (1 + sqrt2)) - dx * 0.05,
               y2_4 - dx * 0.05,
               y2_6 - dx6,
               y2_5 - dx5,
               y2 - dx0};
    item.c = color;
    buffer.push_back('p');
    auto wr = BinaryWriter(buffer);
    item.WriteTo(wr);
  }

  void font_path(const char* s) { font_path_ = s; }
  const std::string& font_path() const { return font_path_; }

  void default_alpha(uint8_t a) { default_alpha_ = a; }
  uint8_t default_alpha() const { return default_alpha_; }

  bool enabled() const { return enabled_; }
  void enabled(bool b) { enabled_ = b; }

 private:
  std::mutex mtx;
  std::vector<char> commands;
  std::vector<int> time_index;
  std::vector<char> buffer;
  int vis_time_index = 0;
  double buffer_time = 0;

  bool initialized = false;
  bool enabled_ = true;
  SDL_Window* window = nullptr;
  SDL_Renderer* renderer = nullptr;
  TTF_Font* font = nullptr;
  uint8_t default_alpha_ = 0xFF;
  std::string font_path_;
  bool auto_mode_ = true;
  double zoom = 1.0;
  BoundingBox<double> content_box;
  RenderArgs<double> render_args;
  Point<int> center;
  int window_width, window_height;

  void Init() {
    if (!enabled()) return;
    if (initialized) return;
    mtx.lock();
    buffer_time = 0;

    buffer.push_back('n');
    BinaryWriter(buffer).Write(buffer_time);
    mtx.unlock();

    SDL_Init(SDL_INIT_EVERYTHING);
    TTF_Init();
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);

    center.x = 0;
    center.y = 0;
    window = SDL_CreateWindow("Visualizer", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, 960, 640,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  bool FontCheck() {
    if (!font && !font_path().empty())
      font = TTF_OpenFont(font_path().c_str(), 64);
    return font != nullptr;
  }

  void Zoom(int direction, bool think_mouse = false) {
    if (direction == 0) return;
    double ratio = std::pow(0.5, direction * 0.080482023721841);
    double new_zoom = std::min(std::max(zoom * ratio, 1.0), 10.0);

    center.x *= new_zoom / zoom;
    center.y *= new_zoom / zoom;
    zoom = new_zoom;
    UpdateCenter();

    if (think_mouse) {
      int mousex, mousey;
      SDL_GetMouseState(&mousex, &mousey);
      int sign = direction > 0 ? 1 : -1;
      int dx = std::round(sign * 0.1 * ((mousex - window_width * 0.5)));
      int dy = std::round(sign * 0.1 * ((mousey - window_height * 0.5)));
      UpdateCenter(dx, dy);
    }
  }

  void FlushLocked() {
    if (buffer.empty()) {
      return;
    }
    if (auto_mode_) {
      vis_time_index = static_cast<int>(time_index.size());
    }
    time_index.push_back(static_cast<int>(commands.size()));
    commands.insert(commands.end(), std::make_move_iterator(buffer.begin()),
                    std::make_move_iterator(buffer.end()));
    buffer.clear();
  }

  void UpdateCenter(int dx = 0, int dy = 0) {
    center.x += dx;
    center.y += dy;
    const auto content_w = content_box.ux - content_box.lx;
    const auto content_h = content_box.uy - content_box.ly;
    const auto scale =
        std::min(window_width / content_w, window_height / content_h);
    const auto px = (window_width - content_w * scale) * 0.5;
    const auto py = (window_height - content_h * scale) * 0.5;
    auto ux = 0.5 * content_w * scale * (zoom - 1) - px;
    auto lx = 0.5 * -content_w * scale * (zoom - 1) + px;
    auto uy = 0.5 * content_h * scale * (zoom - 1) - py;
    auto ly = 0.5 * -content_h * scale * (zoom - 1) + py;
    if (ux < lx) ux = lx = 0;
    if (uy < ly) uy = ly = 0;
    center.x = std::round(std::max(std::min((double)center.x, ux), lx));
    center.y = std::round(std::max(std::min((double)center.y, uy), ly));
  }

  // align_h: r is 0:center, 1:left 2:right
  // align_v: r is 0:center, 1:top, 2:bottom
  void RenderText(double x, double y, double r, int align_h, int align_v,
                  GvColor c, const char* format = "?", ...) {
    SDL_Color col;
    col.r = c.r;
    col.g = c.g;
    col.b = c.b;
    col.a = c.a;

    char buf[1024];
    va_list arg;
    va_start(arg, format);
    auto size = vsnprintf(buf, 256, format, arg);
    va_end(arg);
    if (size < 0) return;

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, buf, col);
    if (surface == nullptr) return;
    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);

    const int w = next_power_of_two(surface->w);
    const int h = next_power_of_two(surface->h);

    SDL_Surface* s = SDL_CreateRGBSurface(0, w, h, 32, 0x00ff0000, 0x0000ff00,
                                          0x000000ff, 0xff000000);
    SDL_Rect blit_rect;
    blit_rect.x = blit_rect.y = 0;
    blit_rect.w = surface->w;
    blit_rect.h = surface->h;
    SDL_Rect blit2_rect;
    blit2_rect.x = (w - surface->w) * 0.5;
    blit2_rect.y = (h - surface->h) * 0.5;
    blit2_rect.w = surface->w;
    blit2_rect.h = surface->h;

    SDL_BlitSurface(surface, &blit_rect, s, &blit2_rect);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 s->pixels);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    auto center = Point<double>(x, y);
    double scale = r / surface->h;
    double lx = center.x - (w * scale * 0.5);
    double ly = center.y - (h * scale * 0.5);
    double ux = center.x + (w * scale * 0.5);
    double uy = center.y + (h * scale * 0.5);
    if (align_h == 1) {  // left
      lx += surface->w * scale * 0.5;
      ux += surface->w * scale * 0.5;
    } else if (align_h == 2) {  // right
      lx -= surface->w * scale * 0.5;
      ux -= surface->w * scale * 0.5;
    }
    if (align_v == 1) {  // top
      ly += surface->h * scale * 0.5;
      uy += surface->h * scale * 0.5;
    } else if (align_v == 2) {  // bottom
      ly -= surface->h * scale * 0.5;
      uy -= surface->h * scale * 0.5;
    }

    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    {
      glTexCoord2d(0, 0);
      glVertex2d(lx, ly);
      glTexCoord2d(1, 0);
      glVertex2d(ux, ly);
      glTexCoord2d(1, 1);
      glVertex2d(ux, uy);
      glTexCoord2d(0, 1);
      glVertex2d(lx, uy);
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
    SDL_FreeSurface(s);
    SDL_FreeSurface(surface);
    glDeleteTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  void Render() {
    render_args.font = font;
    render_args.render_text_func = std::bind(
        &GvSDL::RenderText, this, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4, std::placeholders::_5,
        std::placeholders::_6, std::placeholders::_7);

    SDL_GetWindowSize(window, &window_width, &window_height);

    const auto content_w = content_box.ux - content_box.lx;
    const auto content_h = content_box.uy - content_box.ly;
    double scale =
        std::min(window_width / content_w, window_height / content_h);

    glMatrixMode(GL_PROJECTION);
    glViewport(0, 0, window_width, window_height);

    glLoadIdentity();
    glOrtho(-window_width * 0.5, window_width * 0.5, window_height * 0.5,
            -window_height * 0.5, 0, 16);
    glTranslated(center.x, center.y, 0);
    glScaled(scale * zoom, scale * zoom, 1);
    glTranslated(-content_box.lx - content_w * 0.5,
                 -content_box.ly - content_h * 0.5, 0);

    GvPolygonItem<double> polygon_item;
    GvCircleItem<double> circle_item;
    GvTextItem<double> text_item;

    double vis_time = 0;
    mtx.lock();
    BinaryReader reader(commands, time_index[vis_time_index]);
    bool visit_t = false;
    while (reader.pos() < commands.size()) {
      char cmd;
      reader.Read(cmd);
      if (cmd == 'n' && visit_t) {
        break;
      } else if (cmd == 'n') {
        visit_t = true;
        reader.Read(vis_time);
      } else if (cmd == 'p') {
        GvPolygonItem<double>::ReadFrom(reader, polygon_item);
        polygon_item.Render(render_args);
        content_box.Update(polygon_item);
      } else if (cmd == 'c') {
        reader.Read(circle_item);
        circle_item.Render(render_args);
        content_box.Update(circle_item);
      } else if (cmd == 't') {
        GvTextItem<double>::ReadFrom(reader, text_item);
        if (font == nullptr) {
          std::cerr << "no font" << std::endl;
          continue;
        }
        text_item.Render(render_args);
        content_box.Update(text_item);
      } else {
        std::cerr << "Unknown command" << std::endl;
      }
    }
    auto cur_index = vis_time_index + 1;
    auto max_index = time_index.size();
    mtx.unlock();

    double mousex, mousey;
    MouseWorldPoint(&mousex, &mousey);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-window_width * 0.5, window_width * 0.5, window_height * 0.5,
            -window_height * 0.5, 0, 16);
    RenderText(-window_width * 0.5, window_height * 0.5, 20, 1, 2,
               ColorIndex(1), "Time(%d / %d) Mouse(%f, %f)", cur_index,
               max_index, mousex, mousey);

    SDL_RenderPresent(renderer);
  }

  void MouseWorldPoint(double* x, double* y) {
    int mousex, mousey;
    SDL_GetMouseState(&mousex, &mousey);

    double modelview[16];
    double projection[16];
    int viewport[4];
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    double z;
    gluUnProject(mousex, window_height - mousey, 0.0, modelview, projection,
                 viewport, x, y, &z);
  }

  void MainLoop() {
    bool running = true;
    render_args.renderer = renderer;

    while (running) {
      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
      SDL_RenderClear(renderer);

      for (SDL_Event ev; SDL_PollEvent(&ev);) {
        if (ev.type == SDL_QUIT) {
          running = false;
        }
        if (ev.type == SDL_KEYDOWN) {
          switch (ev.key.keysym.sym) {
            case SDLK_UP:
              Zoom(4);
              break;
            case SDLK_DOWN:
              Zoom(-4);
              break;
            case SDLK_RIGHT:
              if (vis_time_index + 1 < time_index.size()) {
                mtx.lock();
                vis_time_index++;
                auto_mode_ = false;
                mtx.unlock();
              }
              break;
            case SDLK_LEFT:
              if (vis_time_index - 1 >= 0) {
                mtx.lock();
                vis_time_index--;
                auto_mode_ = false;
                mtx.unlock();
              }
              break;
            case SDLK_ESCAPE:
              running = false;
              break;
            default:
              break;
          }
        }
        if (ev.type == SDL_MOUSEMOTION) {
          if (SDL_GetMouseState(NULL, NULL)) {
            UpdateCenter(ev.motion.xrel, ev.motion.yrel);
          }
        }
        if (ev.type == SDL_MOUSEWHEEL) {
          if (ev.wheel.direction == SDL_MOUSEWHEEL_NORMAL) {
            if (ev.wheel.y > 0) {
              Zoom(1, true);
            } else {
              Zoom(-1, true);
            }
          }
        }
      }

      FontCheck();
      Render();
    }
    SDL_Quit();
  }
};

struct GvEmpty {
  GvColor Color(...) { return 0; }
  GvColor ColorIndex(...) { return 0; }
  void Flush(...) {}
  void NewTime(...) {}
  void RunMainThread(std::function<void()> f) { f(); }
  void RunSubThread(...) {}
  void Line(...) {}
  void Circle(...) {}
  void Rect(...) {}
  void Text(...) {}
  void Arrow(...) {}
  void font_path(...) {}
  std::string font_path() { return ""; }
  void default_alpha(...) { return; }
  uint8_t default_alpha() { return 0; }
  bool enabled(...) { return false; }
};

}  // namespace gv_internal

#ifndef DISABLE_GV
static gv_internal::GvSDL gv;
#else
static gv_internal::GvEmpty gv;
#endif