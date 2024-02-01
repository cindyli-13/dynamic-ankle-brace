#pragma once

#include "driver/gpio.h"

class Gpio {
 public:
  Gpio(gpio_num_t pin, bool active_high)
      : pin_(pin), active_high_(active_high) {}

  void init();
  void activate();
  void deactivate();
  void toggle();
  bool is_active();

 private:
  gpio_num_t pin_;
  bool active_high_;
};
