#include "config_manager_task.h"

void ConfigManagerTask::init() {
  int addr_family = AF_INET;
  int ip_protocol = IPPROTO_IP;
  struct sockaddr_storage dest_addr;
  struct sockaddr_in* dest_addr_ip4 = (struct sockaddr_in*)&dest_addr;
  dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
  dest_addr_ip4->sin_family = AF_INET;
  dest_addr_ip4->sin_port = htons(config_port_);

  config_sock_ = socket(addr_family, SOCK_STREAM, ip_protocol);
  if (config_sock_ < 0) {
    printf("Unable to create socket: errno %d\n", errno);
    vTaskDelete(NULL);
  }
  int opt = 1;
  setsockopt(config_sock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  printf("Socket created\n");

  int err = bind(config_sock_, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
  if (err != 0) {
    printf("Socket unable to bind: errno %d\n", errno);
    printf("IPPROTO: %d\n", addr_family);
    close(config_sock_);
    vTaskDelete(NULL);
  }

  printf("Socket bound, port %d\n", config_port_);

  err = listen(config_sock_, 1);
  if (err != 0) {
    printf("Error occurred during listen: errno %d\n", errno);
    close(config_sock_);
    vTaskDelete(NULL);
  }
}

bool ConfigManagerTask::read_control_message() {
  bool ret = true;
  printf("Socket listening\n");
  struct sockaddr_storage source_addr;
  socklen_t addr_len = sizeof(source_addr);
  int sock = accept(config_sock_, (struct sockaddr*)&source_addr, &addr_len);
  if (sock < 0) {
    printf("Unable to accept connection: errno %d\n", errno);
    ret = false;
  } else {
    if (source_addr.ss_family == PF_INET) {
      inet_ntoa_r(((struct sockaddr_in*)&source_addr)->sin_addr, addr_buffer_,
                  shared::MAX_IP_ADDR_SIZE_ - 1);
    }
    printf("Socket accepted ip address: %s\n", addr_buffer_);

    int len;
    do {
      len = recv(sock, rx_buffer_, rx_buffer_size_ - 1, 0);
      if (len < 0) {
        printf("Error occurred during receiving: errno %d\n", errno);
        ret = false;
      } else if (len == 0) {
        printf("Connection closed\n");
      } else {
        rx_buffer_[len] = 0;
        printf("Received %d bytes: %s\n", len, rx_buffer_);
      }
    } while (len > 0);
  }

  shutdown(sock, 0);
  close(sock);
  return ret;
}

void ConfigManagerTask::handle_control_message(Param* task_param) {
  // expected string format is one of:
  // "CFG:inv_thresh:300,act_time:2500,idle_var:1.5,idle_time:60000"
  // "CALIBRATE"
  // "TELEMETRY_START:4243"
  // "TELEMETRY_STOP"
  std::string msg(rx_buffer_);
  size_t cfgPos = msg.find(config_label_);
  size_t telemStartPos = msg.find(telemetry_start_label_);
  size_t telemStopPos = msg.find(telemetry_stop_label_);
  size_t calibrationRequestPos = msg.find(calibration_request_label_);

  if (cfgPos != std::string::npos) {
    msg.erase(cfgPos, 4);
    std::string token;
    std::string delimiter = ",";
    size_t pos = msg.find(delimiter);
    shared::Config config;
    int fields_found = 0;
    while (!msg.empty()) {
      token = msg.substr(0, pos);
      size_t colonPos = token.find(":");
      if (colonPos != std::string::npos) {
        std::string key = token.substr(0, colonPos);
        std::string value = token.substr(colonPos + 1);
        if (key == "inv_thresh") {
          config.inversion_threshold_deg_s = std::stoi(value);
          fields_found++;
        } else if (key == "act_time") {
          config.actuation_timeout_ms = std::stoi(value);
          fields_found++;
        } else if (key == "idle_var") {
          config.idle_variance_threshold = std::stof(value);
          fields_found++;
        } else if (key == "idle_time") {
          config.idle_transition_time_ms = std::stoi(value);
          fields_found++;
        }
      }
      if (pos == std::string::npos) {
        msg.erase(msg.begin(), msg.end());
      } else {
        msg.erase(0, pos + 1);
      }
      pos = msg.find(delimiter);
    }
    if (fields_found == 4) {
      task_param->config_params_buffer->send(config);
    } else {
      printf("Malformed configuration packet received");
    }
  } else if (calibrationRequestPos != std::string::npos) {
    bool request = true;
    task_param->calibration_requested_buffer->send(request);
  } else if (telemStartPos != std::string::npos) {
    msg.erase(telemStartPos, sizeof(telemetry_start_label_) - 1);
    shared::TelemetryControl telemetry_control;
    telemetry_control.start = true;
    telemetry_control.port = static_cast<uint16_t>(std::stoi(msg)),
    memcpy(telemetry_control.addr, addr_buffer_, shared::MAX_IP_ADDR_SIZE_);
    task_param->telemetry_control_buffer->send(telemetry_control);
  } else if (telemStopPos != std::string::npos) {
    shared::TelemetryControl telemetry_control;
    telemetry_control.start = false;
    task_param->telemetry_control_buffer->send(telemetry_control);
  } else {
    printf("Invalid control message received");
  }
}

void ConfigManagerTask::run(void* param) {
  while (true) {
    Param* task_param = static_cast<Param*>(param);
    if (read_control_message()) {
      handle_control_message(task_param);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
