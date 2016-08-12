#include <chrono>
#include <iostream>
#include <thread>
#include "gv.hpp"
using namespace std;

int main(int, char** const) {
  gv.RunMainThread([] {
    //  gv.font_path("/Users/shingo/projects/gv-sdl/Inconsolata-Regular.ttf");
    gv.font_path("/Users/shingo/projects/gv-sdl/MTLmr3m.ttf");
    for (int i = 0; i < 100; ++i) {
      gv.NewTime();

      if (i > 10 && i < 50) {
        gv.Text(100, 100, 5.0, gv.ColorIndex(4), "Hello!!! %d", i);
      }

      gv.default_alpha(128);
      gv.Arrow(1 * i, 10, 200, 200, 10, gv.ColorIndex(i));
      gv.Line(200 - 1 * i, 10, 10, 200, 10, gv.ColorIndex(i));
      gv.Circle(100, 100, 100, false, gv.ColorIndex(i));
      gv.Rect(150, 150, 100, 100, gv.Color(255, 128, 128, 128));
      gv.Flush();
      cout << "hello" << i << endl;
      const std::chrono::milliseconds d(100);
      this_thread::sleep_for(d);
    }
  });
  return 0;
}
