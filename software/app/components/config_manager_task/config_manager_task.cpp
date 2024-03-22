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

void ConfigManagerTask::read_control_message(char rx_buffer[rx_buffer_size_],
                                          char addr_str[addr_buffer_size_]) {
  printf("Socket listening\n");
  struct sockaddr_storage source_addr;
  socklen_t addr_len = sizeof(source_addr);
  int sock = accept(config_sock_, (struct sockaddr*)&source_addr, &addr_len);
  if (sock < 0) {
    printf("Unable to accept connection: errno %d\n", errno);
  } else {
    if (source_addr.ss_family == PF_INET) {
      inet_ntoa_r(((struct sockaddr_in*)&source_addr)->sin_addr, addr_str,
                  addr_buffer_size_ - 1);
    }
    printf("Socket accepted ip address: %s\n", addr_str);

    int len;
    do {
      len = recv(sock, rx_buffer, rx_buffer_size_ - 1, 0);
      if (len < 0) {
        printf("Error occurred during receiving: errno %d\n", errno);
      } else if (len == 0) {
        printf("Connection closed\n");
      } else {
        rx_buffer[len] = 0;
        printf("Received %d bytes: %s\n", len, rx_buffer);
      }
    } while (len > 0);
  }

  shutdown(sock, 0);
  close(sock);
}

void ConfigManagerTask::run(void* param) {
  char rx_buffer[256];
  char addr_str[128];
  while (true) {
    read_control_message(rx_buffer, addr_str);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
