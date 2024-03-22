#pragma once
#include "esp_event.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "shared.h"
#include "string.h"
#include "task.h"


#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#define SSID ("DynamicAnkleSupport")
#define PASSWORD ("tronwasamistake")

class NetworkingTask : public Task {
 public:
  struct Param {
    int dummy;
  };
  NetworkingTask(const Task::Config& config, Param* const param)
      : Task(config, param) {}

 private:
  static constexpr uint8_t channel_ = 11;
  static constexpr uint8_t max_connections_ = 1;
  static constexpr uint16_t config_port_ = 4242;
  static constexpr uint16_t telemetry_port_ = 4243;
  static constexpr size_t rx_buffer_size_ = 256;
  static constexpr size_t addr_buffer_size_ = 128;
  int32_t config_sock_;
  int32_t telemetry_sock_;

  void init();
  void run(void* param);

  void init_wifi();
  void init_config_socket();
  void init_telemetry_socket(const char* dest_address, uint16_t port);
  void read_control_message(char* rx_buffer, char* addr_str);
  static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                 int32_t event_id, void* event_data);
};