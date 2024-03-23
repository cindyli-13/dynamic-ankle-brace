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
  struct Param {
    DataBuffer<float, 10>* inversion_speed_buffer;
    DataBuffer<shared::State, 1>* state_buffer;
    DataBuffer<float, 1>* battery_voltage_buffer;
  };

  TelemetryTask(const Task::Config& config, Param* const param)
      : Task(config, param) {}

 private:
  static constexpr size_t rx_buffer_size_ = 256;
  static constexpr size_t addr_buffer_size_ = 128;
  int32_t telemetry_sock_;
  sockaddr_in dest_addr_;

  void init();
  void run(void* param);
  void init_telemetry_socket(const char* dest_address, uint16_t port);
};