#pragma once
#include <string>
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

class ConfigManagerTask : public Task {
 public:
  struct Param {
    DataBuffer<bool, 1>* calibration_requested_buffer;
    DataBuffer<shared::Config, 1>* config_params_buffer;
    DataBuffer<shared::TelemetryControl, 1>* telemetry_control_buffer;
  };

  ConfigManagerTask(const Task::Config& config, Param* const param)
      : Task(config, param) {}

 private:
  static constexpr uint16_t config_port_ = 4242;
  static constexpr size_t rx_buffer_size_ = 256;
  static constexpr char config_label_[] = "CFG:";
  static constexpr char telemetry_start_label_[] = "TELEMETRY_START:";
  static constexpr char telemetry_stop_label_[] = "TELEMETRY_STOP";
  static constexpr char calibration_request_label_[] = "CALIBRATE";

  char rx_buffer_[rx_buffer_size_];
  char addr_buffer_[shared::MAX_IP_ADDR_SIZE_];
  int32_t config_sock_;

  void init();
  void run(void* param);
  bool read_control_message();
  void handle_control_message(Param* task_param);
};