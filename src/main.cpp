#include <DoorstepDefender/Hardware/VL6180X.h>
#include <iostream>
#include <fmt/core.h>
#include <thread>
#include <chrono>

int main() {
  using namespace std::literals::chrono_literals;
  VL6180X tof { 1 };
  while (true) {
    fmt::print("Range: {}\n", tof.get_range());
    std::this_thread::sleep_for(50ms);
  }
  return 0;
}
