#include <chrono>
#include <iostream>
#include <thread>
#include "gv.hpp"
using namespace std;

int main(int, char** const) {
  gv.RunMainThread([] {
    //  gv.font_path("/Users/shingo/projects/gv-sdl/Inconsolata-Regular.ttf");
    gv.font_path("/Users/shingo/projects/gv-sdl/MTLmr3m.ttf");
    while (true) {
      for (int i = 0; i < 100; ++i) {
        gv.NewTime();

        // gv.Rect(100, 100, 100, 100, gv.ColorIndex(23));
        // gv.Rect(200, 200, 100, 100, gv.ColorIndex(23));

        gv.Line(0, -100, 0, 100, 10, gv.ColorIndex(9));
        gv.Line(-100, 0, 100, 0, 10, gv.ColorIndex(9));

        gv.Text(100, 100, 50, gv.ColorIndex(4), "Hello!!!%d%d", i + 10, i);
        gv.Text(100, 100, 50, gv.ColorIndex(4), "こんにちはーーー", i + 10, i);
        gv.Line(0, 100, 200, 100, 10, gv.ColorIndex(9));
        gv.Rect(-100, -100, 200, 200, gv.ColorIndex(23));
        gv.Line(100, 0, 100, 200, 10, gv.ColorIndex(9));
        gv.Rect(64, 75, 136 - 64, 125 - 75, gv.ColorIndex(10));

        gv.default_alpha(128);
        gv.Arrow(1 * i, 10, 200 + 1 * i, 200, 10, gv.ColorIndex(0));
        gv.Line(200 - 1 * i, 10, 10, 200, 10, gv.ColorIndex(9));
        gv.Circle(100, 100, 100, false, gv.ColorIndex(2));
        gv.Rect(150, 150, i * 20, i * 10, gv.Color(255, 128, 128, 128));
        gv.Flush();
        // cout << "hello" << i << endl;
        const std::chrono::milliseconds d(10);
        this_thread::sleep_for(d);
      }
    }
  });
  return 0;
}
