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
    DataBuffer<shared::StampedInversionSpeed, 20>* inversion_speed_buffer;
    DataBuffer<shared::State, 1>* state_buffer;
    DataBuffer<float, 1>* battery_voltage_buffer;
    DataBuffer<shared::Config, 1>* internal_config_buffer;
    DataBuffer<shared::TelemetryControl, 1>* telemetry_control_buffer;
  };

  TelemetryTask(const Task::Config& config, Param* const param)
      : Task(config, param) {}

 private:
  static constexpr size_t tx_buffer_size_ = 1024;
  char tx_buffer_[tx_buffer_size_];
  int32_t telemetry_sock_;
  sockaddr_in dest_addr_;
  bool active_;
  std::string dest_ip_addr_;

  void init();
  void run(void* param);
  const char* stateToString(shared::State);
  bool open_telemetry_socket(const char* dest_address, uint16_t port);
  bool close_telemetry_socket();
};