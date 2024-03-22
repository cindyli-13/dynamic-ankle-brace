#include "networking_task.h"

void NetworkingTask::wifi_event_handler(void* arg, esp_event_base_t event_base,
                                        int32_t event_id, void* event_data) {
  if (event_id == WIFI_EVENT_AP_STACONNECTED) {
    wifi_event_ap_staconnected_t* event =
        (wifi_event_ap_staconnected_t*)event_data;
    printf("station %d:%d:%d:%d:%d:%d join, AID=%d\n", MAC2STR(event->mac),
           event->aid);
  } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    wifi_event_ap_stadisconnected_t* event =
        (wifi_event_ap_stadisconnected_t*)event_data;
    printf("station %d:%d:%d:%d:%d:%d leave, AID=%d\n", MAC2STR(event->mac),
           event->aid);
  }
}

void NetworkingTask::init() {
  init_wifi();
  // init_config_socket();
}

void NetworkingTask::init_wifi() {
  // initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

  wifi_config_t wifi_config = {
      .ap =
          {
              .ssid = SSID,
              .password = PASSWORD,
              .channel = channel_,
              .authmode = WIFI_AUTH_WPA2_PSK,
              .max_connection = max_connections_,
              .pmf_cfg =
                  {
                      .required = true,
                  },
          },
  };

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  printf("WiFi SoftAP Initialized with SSID: %s password: %s channel: %d\n",
         SSID, PASSWORD, channel_);
}

// void NetworkingTask::init_config_socket() {
//   int addr_family = AF_INET;
//   int ip_protocol = IPPROTO_IP;
//   struct sockaddr_storage dest_addr;
//   struct sockaddr_in* dest_addr_ip4 = (struct sockaddr_in*)&dest_addr;
//   dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
//   dest_addr_ip4->sin_family = AF_INET;
//   dest_addr_ip4->sin_port = htons(config_port_);

//   config_sock_ = socket(addr_family, SOCK_STREAM, ip_protocol);
//   if (config_sock_ < 0) {
//     printf("Unable to create socket: errno %d\n", errno);
//     vTaskDelete(NULL);
//   }
//   int opt = 1;
//   setsockopt(config_sock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
//   printf("Socket created\n");

//   int err = bind(config_sock_, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
//   if (err != 0) {
//     printf("Socket unable to bind: errno %d\n", errno);
//     printf("IPPROTO: %d\n", addr_family);
//     close(config_sock_);
//     vTaskDelete(NULL);
//   }

//   printf("Socket bound, port %d\n", config_port_);

//   err = listen(config_sock_, 1);
//   if (err != 0) {
//     printf("Error occurred during listen: errno %d\n", errno);
//     close(config_sock_);
//     vTaskDelete(NULL);
//   }
// }

// void NetworkingTask::init_telemetry_socket(const char* dest_address,
//                                            uint16_t port) {
//   const char* payload = "Hello, Laptop!";
//   int addr_family = 0;
//   int ip_protocol = 0;

//   struct sockaddr_in dest_addr;
//   dest_addr.sin_addr.s_addr = inet_addr(dest_address);
//   dest_addr.sin_family = AF_INET;
//   dest_addr.sin_port = htons(port);
//   addr_family = AF_INET;
//   ip_protocol = IPPROTO_IP;

//   telemetry_sock_ = socket(addr_family, SOCK_DGRAM, ip_protocol);
//   if (telemetry_sock_ < 0) {
//     printf("Unable to create socket: errno %d", errno);
//     vTaskDelete(NULL);
//   }

//   printf("Socket created, sending to %s:%d", dest_address, port);



//   while (1) {
//     int err = sendto(telemetry_sock_, payload, strlen(payload), 0,
//                      (struct sockaddr*)&dest_addr, sizeof(dest_addr));
//     if (err < 0) {
//       printf("Error occurred during sending: errno %d", errno);
//       break;
//     }
//     printf("Message sent");

//     vTaskDelay(2000 / portTICK_PERIOD_MS);
//   }
// }

// void NetworkingTask::read_control_message(char rx_buffer[rx_buffer_size_],
//                                           char addr_str[addr_buffer_size_]) {
//   printf("Socket listening\n");
//   struct sockaddr_storage source_addr;
//   socklen_t addr_len = sizeof(source_addr);
//   int sock = accept(config_sock_, (struct sockaddr*)&source_addr, &addr_len);
//   if (sock < 0) {
//     printf("Unable to accept connection: errno %d\n", errno);
//   } else {
//     if (source_addr.ss_family == PF_INET) {
//       inet_ntoa_r(((struct sockaddr_in*)&source_addr)->sin_addr, addr_str,
//                   addr_buffer_size_ - 1);
//     }
//     printf("Socket accepted ip address: %s\n", addr_str);

//     int len;
//     do {
//       len = recv(sock, rx_buffer, rx_buffer_size_ - 1, 0);
//       if (len < 0) {
//         printf("Error occurred during receiving: errno %d\n", errno);
//       } else if (len == 0) {
//         printf("Connection closed\n");
//       } else {
//         rx_buffer[len] = 0;
//         printf("Received %d bytes: %s\n", len, rx_buffer);
//         init_telemetry_socket(addr_str, telemetry_port_);
//       }
//     } while (len > 0);
//   }

//   shutdown(sock, 0);
//   close(sock);
// }

void NetworkingTask::run(void* param) {
  // char rx_buffer[256];
  // char addr_str[128];
  while (true) {
    // read_control_message(rx_buffer, addr_str);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
