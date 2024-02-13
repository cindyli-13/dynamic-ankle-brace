#include <inttypes.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpio.h"
#include "rgb_led.h"
#include "sdkconfig.h"

enum class LedState {
  kOff,
  kRed,
  kGreen,
  kBlue,
};

extern "C" {

void app_main(void) {
  printf("Blinky\n");
  RgbLed led(DEV_BOARD_LED_PIN);
  led.init();
  LedState led_state = LedState::kOff;

  while (true) {
    switch (led_state) {
      case LedState::kOff:
        led.set(0, 0, 0);
        printf("led off\n");
        led_state = LedState::kRed;
        break;
      case LedState::kRed:
        led.set(16, 0, 0);
        printf("led red\n");
        led_state = LedState::kGreen;
        break;
      case LedState::kGreen:
        led.set(0, 16, 0);
        printf("led green\n");
        led_state = LedState::kBlue;
        break;
      case LedState::kBlue:
        led.set(0, 0, 16);
        printf("led blue\n");
        led_state = LedState::kOff;
        break;
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
}
