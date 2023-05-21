#include <DoorstepDefender/Bluetooth/BluetoothHandler.h>
#include <fmt/core.h>
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <sys/socket.h>
#include <cstring>
#include <cerrno>

static constexpr bdaddr_t __BDADDR_ANY { 0, 0, 0, 0, 0, 0 };

BluetoothHandler::BluetoothHandler() 
: conn_thread([&]() { this->conn_thread_main(); }) { }

BluetoothHandler::~BluetoothHandler() {
  terminate();
  conn_thread.join();
}

void BluetoothHandler::send_status(StatusMessage new_msg) {
  std::lock_guard<std::mutex> lk(conn_mutex);
  new_status = true;
  msg = new_msg;
}

bool BluetoothHandler::is_connected() {
  std::lock_guard<std::mutex> lk(conn_mutex);
  return connected;
}

void BluetoothHandler::terminate() {
  std::lock_guard<std::mutex> lk(conn_mutex);
  should_term = true;
}

void BluetoothHandler::conn_thread_main() {
  using namespace std::literals::chrono_literals;
  
  struct sockaddr_rc loc_addr, conn_addr;
  
  std::memset(&loc_addr, 0, sizeof(struct sockaddr_rc));
  std::memset(&conn_addr, 0, sizeof(struct sockaddr_rc));
  
  loc_addr.rc_family = AF_BLUETOOTH;
  loc_addr.rc_bdaddr = __BDADDR_ANY;
  loc_addr.rc_channel = 1;
  
  int loc_fd, conn_fd;
  
  uint8_t send_buf[1];
  
  // Create local socket for incoming connections.
SOCKET_CREATE:
  {
    std::lock_guard<std::mutex> lk(conn_mutex);
    if (should_term) return;
  }
  
  loc_fd = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
  if (loc_fd < 0) {
    fmt::print("BluetoothHandler: socket() failed\n\t{}\n", strerror(errno));
    
    std::this_thread::sleep_for(1s);
    goto SOCKET_CREATE;
  }
  
  fmt::print("BluetoothHandler: Created local socket\n");
  
  // Bind the socket to the local Bluetooth adapter.
SOCKET_BIND:
  {
    std::lock_guard<std::mutex> lk(conn_mutex);
    if (should_term) {
      close(loc_fd);
      return;
    }
  }
  
  if (bind(loc_fd, (struct sockaddr*)&loc_addr, sizeof(struct sockaddr_rc)) < 0) {
    fmt::print("BluetoothHandler: bind() failed\n\t{}\n", strerror(errno));
    
    std::this_thread::sleep_for(1s);
    goto SOCKET_BIND;
  }
  
  fmt::print("BluetoothHandler: Bound local socket\n");
  
SOCKET_LISTEN:
  {
    std::lock_guard<std::mutex> lk(conn_mutex);
    if (should_term) {
      close(loc_fd);
      return;
    }
  }
  
  if (listen(loc_fd, 1) < 0) {
    fmt::print("BluetoothHandler: listen() failed\n\t{}\n", strerror(errno));
    
    std::this_thread::sleep_for(1s);
    goto SOCKET_LISTEN;
  }
  
  fmt::print("BluetoothHandler: Local socket now listening\n");
  
ACCEPT:
  bool just_accepted = false;
  {
    std::lock_guard<std::mutex> lk(conn_mutex);
    if (should_term) {
      close(loc_fd);
      return;
    }
  }
  
  socklen_t conn_addr_len = sizeof(conn_addr);
  conn_fd = accept(loc_fd, (struct sockaddr*)&conn_addr, &conn_addr_len);
  if (conn_fd < 0) {
    fmt::print("BluetoothHandler: accept() failed\n\t{}\n", strerror(errno));

    std::this_thread::sleep_for(0.1s);
    goto ACCEPT;
  }
  just_accepted = true;
  
  char conn_addr_str[18];
  ba2str(&conn_addr.rc_bdaddr, conn_addr_str);
  fmt::print("BluetoothHandler: Accepted connection from {}\n", conn_addr_str);
  
  std::this_thread::sleep_for(5s);
  
  while (true) {
    bool should_send = false;
    ssize_t sent = 0;
    {
      std::lock_guard<std::mutex> lk(conn_mutex);
      should_send = new_status || just_accepted;
      if (should_term) break;
    }
    
    if (!should_send) goto PAUSE_AND_RETRY;
    
    {
      std::lock_guard<std::mutex> lk(conn_mutex);
      std::memcpy(send_buf, &msg, 1);
    }
    
    sent = write(conn_fd, send_buf, 1);
    if (sent < 0) {
      fmt::print("BluetoothHandler: write() failed\n\t{}\n", strerror(errno));
      if (errno == ECONNRESET) {
        fmt::print("BluetoothHandler: Connection reset by peer\n");
        close(conn_fd);
        goto ACCEPT;
      }
      goto PAUSE_AND_RETRY;
    }
    
    {
      std::lock_guard<std::mutex> lk(conn_mutex);
      new_status = false;
    }
    
PAUSE_AND_RETRY:
    std::this_thread::sleep_for(50ms);
    
    just_accepted = false;
  }
  
  close(conn_fd);
  close(loc_fd);
}
