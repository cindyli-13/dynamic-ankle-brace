#pragma once
#include "gpio.h"
#include "led_strip.h"

static constexpr gpio_num_t DEV_BOARD_LED_PIN = GPIO_NUM_38;

class RgbLed {
 public:
  RgbLed(gpio_num_t pin = DEV_BOARD_LED_PIN) : pin_(pin) {}

  void init() {
    led_strip_config_t strip_config;
    strip_config.strip_gpio_num = pin_;
    strip_config.max_leds = 1;
    strip_config.led_pixel_format = LED_PIXEL_FORMAT_GRB;
    strip_config.led_model = LED_MODEL_WS2812;

    led_strip_rmt_config_t rmt_config;
    rmt_config.clk_src = RMT_CLK_SRC_DEFAULT;
    rmt_config.resolution_hz = 10 * 1000 * 1000;  // 10MHz

    ESP_ERROR_CHECK(
        led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip_));
    led_strip_clear(led_strip_);
  }

  void set(uint8_t r, uint8_t g, uint8_t b) {
    led_strip_set_pixel(led_strip_, 0, r, g, b);
    led_strip_refresh(led_strip_);
  }

  void clear() { led_strip_clear(led_strip_); }

 private:
  gpio_num_t pin_;
  led_strip_handle_t led_strip_;
};