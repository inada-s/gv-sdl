#include <chrono>
#include <iostream>
#include <thread>
#include "gv.hpp"
using namespace std;
using namespace gv;

int main(int, char** const) {
  gvRunMainThread([] {
    for (int i = 0; i < 10; ++i) {
      gvNewTime();

      gvLine(10 * i, 10, 400, 400, Color(255, 128, 128, 128));
      cout << "hello" << i << endl;
      const std::chrono::milliseconds d(1000);
      this_thread::sleep_for(d);
    }
  });
  return 0;
}
