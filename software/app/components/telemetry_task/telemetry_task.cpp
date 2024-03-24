#include "telemetry_task.h"

void TelemetryTask::init() {
  active_ = false;
}

bool TelemetryTask::open_telemetry_socket(const char* dest_address,
                                          uint16_t port) {

  dest_ip_addr_ = dest_address;
  int addr_family = 0;
  int ip_protocol = 0;

  dest_addr_.sin_addr.s_addr = inet_addr(dest_ip_addr_.c_str());
  dest_addr_.sin_family = AF_INET;
  dest_addr_.sin_port = htons(port);
  addr_family = AF_INET;
  ip_protocol = IPPROTO_IP;

  telemetry_sock_ = socket(addr_family, SOCK_DGRAM, ip_protocol);
  if (telemetry_sock_ < 0) {
    printf("Unable to create socket: errno %d", errno);
    return false;
  }

  printf("Socket created, sending to %s:%d\n", dest_ip_addr_.c_str(), port);
  return true;
}

bool TelemetryTask::close_telemetry_socket() {
  bool ret = true;
  if (close(telemetry_sock_) < 0) {
    ret = false;
  }
  return ret;
}

const char* TelemetryTask::stateToString(shared::State state) {
  switch (state) {
    case shared::State::kIdle:
      return "kIdle";
    case shared::State::kActive:
      return "kActive";
    case shared::State::kActuated:
      return "kActuated";
    case shared::State::kCalibrating:
      return "kCalibrating";
  }
  return "kUnknown";
}

void TelemetryTask::run(void* param) {
  Param* task_param = static_cast<Param*>(param);

  while (1) {
    shared::TelemetryControl telemetry_control;
    if (task_param->telemetry_control_buffer->receive(telemetry_control)) {
      if (telemetry_control.start && !active_) {
        if (open_telemetry_socket(telemetry_control.addr,
                                  telemetry_control.port)) {
          active_ = true;
        } else {
          printf("Retrying telemetry init\n");
          // push control message back into buffer for retry
          task_param->telemetry_control_buffer->send(telemetry_control);
        }
      } else if (!telemetry_control.start && active_) {
        if (close_telemetry_socket()) {
          active_ = false;
        } else {
          printf("Retrying telemetry socket close\n");
          // push control message back into buffer for retry
          task_param->telemetry_control_buffer->send(telemetry_control);
        }
      }
    }

    if (active_) {
      int offset = 0;
      offset += snprintf(tx_buffer_ + offset, tx_buffer_size_ - offset, "INV:");
      float inversion_speed;
      while (task_param->inversion_speed_buffer->receive(inversion_speed)) {
        offset += snprintf(tx_buffer_ + offset, tx_buffer_size_ - offset,
                           "%.2f,", inversion_speed);
      }

      shared::State state;
      task_param->state_buffer->peek(state);
      offset += snprintf(tx_buffer_ + offset, tx_buffer_size_ - offset,
                         "ST:%s,", stateToString(state));

      float vbat;
      if (task_param->battery_voltage_buffer->peek(vbat)) {
        offset += snprintf(tx_buffer_ + offset, tx_buffer_size_ - offset,
                           "VB:%.2f,", vbat);
      }

      shared::Config config;
      if (task_param->internal_config_buffer->peek(config)) {
        offset += snprintf(
            tx_buffer_ + offset, tx_buffer_size_ - offset,
            "inv_thresh:%d,act_time:%d,idle_var:%.2f,idle_time:%d",
            config.inversion_threshold_deg_s, config.actuation_timeout_ms,
            config.idle_variance_threshold, config.idle_transition_time_ms);
      }

      int err = sendto(telemetry_sock_, tx_buffer_, strlen(tx_buffer_), 0,
                       (struct sockaddr*)&dest_addr_, sizeof(dest_addr_));
      if (err < 0) {
        printf("Error sending telemetry: errno %d\n", errno);
      }
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);  // run at 100 Hz
  }
}
