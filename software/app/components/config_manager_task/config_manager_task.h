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

class ConfigManagerTask : public Task {
 public:

  ConfigManagerTask(const Task::Config& config)
      : Task(config, nullptr) {}

 private:
  static constexpr uint16_t config_port_ = 4242;
  static constexpr size_t rx_buffer_size_ = 256;
  static constexpr size_t addr_buffer_size_ = 128;
  int32_t config_sock_;

  void init();
  void run(void* param);
  void read_control_message(char* rx_buffer, char* addr_str);
};