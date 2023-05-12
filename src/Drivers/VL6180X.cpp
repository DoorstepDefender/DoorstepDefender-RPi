#include <DoorstepDefender/Drivers/VL6180X.h>
#include <iostream>
#include <fcntl.h>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <fmt/core.h>

VL6180X::VL6180X(int adapter_nr, uint8_t addr) {
  std::string filename = fmt::format("/dev/i2c-{}", adapter_nr);
  
  i2c_fd = open(filename.c_str(), O_RDWR);
  connected = i2c_fd != -1;
  if (!connected) {
    fmt::print("Failed to open I2C device at '{}'\n\t{}\n", filename, strerror(errno));
    return;
  }
  else {
    fmt::print("Successfully opened I2C device at '{}'\n", filename);
  }

  if (ioctl(i2c_fd, I2C_SLAVE, addr) == -1) {
    connected = false;
    fmt::print("Failed to acquire I2C bus address and/or talk to slave.\n\t{}\n", strerror(errno));
    return;
  }
}

VL6180X::~VL6180X() {
}

double VL6180X::get_range() {
  
}

uint8_t VL6180X::readRegister(uint16_t reg) {
  if (!connected) return 0;
  
  uint8_t buffer[3];
  buffer[0] = uint8_t(reg >> 8);
  buffer[1] = uint8_t(reg & 0xFF);

  // Prompt the sensor with the address to read.
  write(i2c_fd, buffer, 2);

  // Read the value at the address.
  bool res = read(i2c_fd, &buffer[2], 1);
  if (res) {
    fmt::print("Failed to read from register {::#x}\n", reg);
  }
  return buffer[2];
}

void VL6180X::writeRegister(uint16_t reg, uint8_t data) {
  if (!connected) return;
  
  uint8_t buffer[3];

  buffer[0] = uint8_t(reg >> 8);
  buffer[1] = uint8_t(reg & 0xFF);
  buffer[2] = data;

  // Write data to register at address.
  if (write(i2c_fd, buffer, 3) != 3) {
    fmt::print("Failed to write to register {::#x}\n", reg);
  }
}
