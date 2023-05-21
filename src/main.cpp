#include <DoorstepDefender/Hardware/VL6180X.h>
#include <DoorstepDefender/Bluetooth/BluetoothHandler.h>
#include <iostream>
#include <fmt/core.h>
#include <thread>
#include <chrono>

int main() {
  using namespace std::literals::chrono_literals;
  VL6180X tof { 1 };
  bool was_on_porch = false;

  BluetoothHandler bt;

  while (true) {
    std::this_thread::sleep_for(50ms);
    uint8_t range = tof.get_range();
    if (range < 40 && !was_on_porch) {
      fmt::print("Package delivered\n");
      bt.send_status(BluetoothHandler::StatusMessage::DELIVERED);
      was_on_porch = true;
    }
    else if (range >= 40 && was_on_porch) {
      fmt::print("Package left\n");
      bt.send_status(BluetoothHandler::StatusMessage::GONE);
      was_on_porch = false;
    }
  }
  return 0;
}
