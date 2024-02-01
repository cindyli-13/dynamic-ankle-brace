#include <inttypes.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpio.h"
#include "sdkconfig.h"

// Need to connect an external LED to this pin
static constexpr gpio_num_t LED_PIN = GPIO_NUM_38;

extern "C" {

void app_main(void) {
  printf("Blinky\n");

  Gpio led(LED_PIN, true);
  led.init();

  while (true) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    led.toggle();
  }
}
}
