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
  const char* payload = "Hello, Laptop!";
  while (1) {
    int err = sendto(telemetry_sock_, payload, strlen(payload), 0,
                     (struct sockaddr*)&dest_addr_, sizeof(dest_addr_));
    if (err < 0) {
      printf("Error occurred during sending: errno %d", errno);
      break;
    }
    printf("Message sent\n");

    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}
