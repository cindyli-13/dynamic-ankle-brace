#include "telemetry_task.h"

void TelemetryTask::init() {
  init_telemetry_socket("192.168.4.2", 4243);
}

void TelemetryTask::init_telemetry_socket(const char* dest_address,
                                          uint16_t port) {
  int addr_family = 0;
  int ip_protocol = 0;

  dest_addr_.sin_addr.s_addr = inet_addr(dest_address);
  dest_addr_.sin_family = AF_INET;
  dest_addr_.sin_port = htons(port);
  addr_family = AF_INET;
  ip_protocol = IPPROTO_IP;

  telemetry_sock_ = socket(addr_family, SOCK_DGRAM, ip_protocol);
  if (telemetry_sock_ < 0) {
    printf("Unable to create socket: errno %d", errno);
    vTaskDelete(NULL);
  }

  printf("Socket created, sending to %s:%d", dest_address, port);
}

void TelemetryTask::run(void* param) {
  Param* task_param = static_cast<Param*>(param);

  while (1) {
    int offset = 0;
    offset += snprintf(tx_buffer_ + offset, tx_buffer_size_ - offset, "INV:");
    float inversion_speed;
    while (task_param->inversion_speed_buffer->receive(inversion_speed)) {
      offset += snprintf(tx_buffer_ + offset, tx_buffer_size_ - offset, "%.2f,",
                         inversion_speed);
    }

    shared::State state;
    task_param->state_buffer->peek(state);
    offset += snprintf(tx_buffer_ + offset, tx_buffer_size_ - offset, "ST:%s,",
                       stateToString(state));

    float vbat;
    task_param->battery_voltage_buffer->peek(vbat);
    offset += snprintf(tx_buffer_ + offset, tx_buffer_size_ - offset,
                       "VB:%.2f,", vbat);

    int err = sendto(telemetry_sock_, tx_buffer_, strlen(tx_buffer_), 0,
                     (struct sockaddr*)&dest_addr_, sizeof(dest_addr_));
    if (err < 0) {
      printf("Error occurred during sending: errno %d", errno);
      break;
    }
    printf("Message sent\n");

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
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
