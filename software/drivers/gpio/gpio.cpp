#include "gpio.h"

void Gpio::init() {
  gpio_config_t gpio_cfg = {
      .pin_bit_mask = (1ULL << pin_),
      .mode = GPIO_MODE_INPUT_OUTPUT,
      .pull_up_en = (active_high_ ? GPIO_PULLUP_DISABLE : GPIO_PULLUP_ENABLE),
      .pull_down_en =
          (active_high_ ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE),
      .intr_type = GPIO_INTR_DISABLE};

  ESP_ERROR_CHECK(gpio_config(&gpio_cfg));
}

void Gpio::activate() {
  ESP_ERROR_CHECK(gpio_set_level(pin_, active_high_));
}

void Gpio::deactivate() {
  ESP_ERROR_CHECK(gpio_set_level(pin_, !active_high_));
}

void Gpio::toggle() {
  ESP_ERROR_CHECK(gpio_set_level(pin_, !gpio_get_level(pin_)));
}

bool Gpio::is_active() {
  return gpio_get_level(pin_) == active_high_;
}
