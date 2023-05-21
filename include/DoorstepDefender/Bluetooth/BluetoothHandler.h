#pragma once

#include <thread>
#include <mutex>
#include <cstdint>

class BluetoothHandler {
public:
  enum class StatusMessage {
    DELIVERED = 'd',
    GONE = 'g',
  };
  
  BluetoothHandler();
  ~BluetoothHandler();
  
  /**
   * @brief Sends a status message to the connected device.
   *
   * @param msg The message to send.
   */
  void send_status(StatusMessage msg);
  
  /**
   * @brief Returns whether a device is connected.
   *
   * @return Whether a device is connected.
   */
  bool is_connected();
  
  /**
   * @brief Terminate the connection thread.
   */
  void terminate();
  
private:
  std::thread conn_thread;
  std::mutex conn_mutex;
  
  // Connection thread main function.
  void conn_thread_main();
  
  bool connected = false;
  bool should_term = false;
  
  bool new_status = false;
  StatusMessage msg = StatusMessage::GONE;
};
