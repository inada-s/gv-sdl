#include <chrono>
#include <iostream>
#include <thread>
#include "gv.hpp"
using namespace std;

int main(int, char** const) {
  gvMain([] {
    for (int i = 0; i < 10; ++i) {
      cout << "hello" << i << endl;
      const std::chrono::milliseconds d(1000);
      this_thread::sleep_for(d);
      gvLine(10 * i, 10, 400, 400);
    }
  });
  return 0;
}
