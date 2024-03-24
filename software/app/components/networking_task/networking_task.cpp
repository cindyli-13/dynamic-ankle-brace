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

void NetworkingTask::run(void* param) {
  while (true) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
