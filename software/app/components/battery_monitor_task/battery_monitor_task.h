#pragma once

#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "gpio.h"
#include "task.h"

class BatteryMonitorTask : public Task {
 public:
  enum class BatteryState {
    Low,
    Good,
    Unknown,
  };

  BatteryMonitorTask(const Task::Config& config)
      : Task(config, nullptr),
        state_(BatteryState::Unknown),
        green_led_(PIN_NUM_GREEN_LED, true),
        red_led_(PIN_NUM_RED_LED, true),
        adc_(nullptr),
        adc_cali_(nullptr) {}

 private:
  static constexpr gpio_num_t PIN_NUM_GREEN_LED = GPIO_NUM_5;
  static constexpr gpio_num_t PIN_NUM_RED_LED = GPIO_NUM_6;

  static constexpr adc_channel_t ADC_CHANNEL = ADC_CHANNEL_1;
  static constexpr adc_unit_t ADC_UNIT = ADC_UNIT_1;
  static constexpr adc_atten_t ADC_ATTENUATION = ADC_ATTEN_DB_11;
  static constexpr adc_bitwidth_t ADC_BITWIDTH = ADC_BITWIDTH_DEFAULT;

  static constexpr int BATTERY_GOOD_THRESHOLD_MV = 3200;
  static constexpr float VBATT_MONITOR_TO_VBATT = 1.304f;

  BatteryState state_;

  Gpio green_led_;
  Gpio red_led_;

  adc_oneshot_unit_handle_t adc_;
  adc_cali_handle_t adc_cali_;

  void init_adc();
  void update_state(int vbatt_mV);

  void init();
  void run(void* param);
};
