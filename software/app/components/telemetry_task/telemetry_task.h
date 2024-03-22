#pragma once
#include "esp_event.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "shared.h"
#include "string.h"
#include "task.h"


#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

class TelemetryTask : public Task {
 public:

  TelemetryTask(const Task::Config& config)
      : Task(config, nullptr) {}

 private:
  static constexpr size_t rx_buffer_size_ = 256;
  static constexpr size_t addr_buffer_size_ = 128;
  int32_t telemetry_sock_;
  sockaddr_in dest_addr_;

  void init();
  void run(void* param);
  void init_telemetry_socket(const char* dest_address, uint16_t port);
};