#pragma once

#include "esp_adc/adc_continuous.h"
#include "freertos/semphr.h"
#include "gpio.h"
#include "task.h"

class BatteryMonitorTask : public Task {
 public:
  BatteryMonitorTask(const Task::Config& config)
      : Task(config, nullptr),
        green_led_(PIN_NUM_GREEN_LED, true),
        red_led_(PIN_NUM_RED_LED, true),
        adc_(nullptr) {}

 private:
  static bool IRAM_ATTR adc_callback(adc_continuous_handle_t handle,
                                     const adc_continuous_evt_data_t* edata,
                                     void* user_data);

  static constexpr gpio_num_t PIN_NUM_GREEN_LED = GPIO_NUM_5;
  static constexpr gpio_num_t PIN_NUM_RED_LED = GPIO_NUM_6;
  static constexpr gpio_num_t PIN_NUM_ADC = GPIO_NUM_2;

  static constexpr adc_channel_t ADC_CHANNEL = ADC_CHANNEL_1;
  static constexpr adc_unit_t ADC_UNIT = ADC_UNIT_1;
  static constexpr adc_atten_t ADC_ATTENUATION = ADC_ATTEN_DB_0;

  uint8_t adc_result_[4] = {0};

  Gpio green_led_;
  Gpio red_led_;

  adc_continuous_handle_t adc_;

  SemaphoreHandle_t adc_sem_;
  StaticSemaphore_t adc_sem_buffer_;

  void init();
  void run(void* param);
};
