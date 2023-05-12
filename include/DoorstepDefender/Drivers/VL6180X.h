#pragma once

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstdint>

class VL6180X {
public:
  enum {
    DEFAULT_ADDR = 0x29,
  };

  VL6180X(int adapter_nr, uint8_t addr = DEFAULT_ADDR);
  ~VL6180X();

  constexpr bool is_connected() const { return connected; }

private:
  uint8_t readRegister(uint16_t reg);
  void writeRegister(uint16_t reg, uint8_t data);

  int i2c_fd;

  bool connected;
};
