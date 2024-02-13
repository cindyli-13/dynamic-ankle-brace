#include "battery_monitor_task.h"

void BatteryMonitorTask::init() {
  adc_oneshot_unit_init_cfg_t adc_init_cfg = {
      .unit_id = ADC_UNIT,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_init_cfg, &adc_));

  adc_oneshot_chan_cfg_t adc_channel_cfg = {
      .atten = ADC_ATTENUATION,
      .bitwidth = ADC_BITWIDTH,
  };
  ESP_ERROR_CHECK(
      adc_oneshot_config_channel(adc_, ADC_CHANNEL, &adc_channel_cfg));

  adc_cali_curve_fitting_config_t adc_cali_cfg = {
      .unit_id = ADC_UNIT,
      .chan = ADC_CHANNEL,
      .atten = ADC_ATTENUATION,
      .bitwidth = ADC_BITWIDTH,
  };
  ESP_ERROR_CHECK(
      adc_cali_create_scheme_curve_fitting(&adc_cali_cfg, &adc_cali_));

  green_led_.init();
  red_led_.init();
}

void BatteryMonitorTask::run(void* param) {
  int adc_raw_data;
  int vbatt_monitor_mV;
  while (true) {
    ESP_ERROR_CHECK(adc_oneshot_read(adc_, ADC_CHANNEL, &adc_raw_data));
    ESP_ERROR_CHECK(
        adc_cali_raw_to_voltage(adc_cali_, adc_raw_data, &vbatt_monitor_mV));

    // Need to convert from vbatt_monitor to vbatt due to voltage divider circuit
    // Also apply exponential moving average filter to vbatt measurement
    vbatt_mV_ = (VBATT_EMA_FILTER_ALPHA * vbatt_mV_) +
                (vbatt_monitor_mV * VBATT_MONITOR_TO_VBATT *
                 (1 - VBATT_EMA_FILTER_ALPHA));

    update_state();

    // Run at 1Hz
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  ESP_ERROR_CHECK(adc_oneshot_del_unit(adc_));
  ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(adc_cali_));
}

void BatteryMonitorTask::update_state() {
  if (vbatt_mV_ > BATTERY_GOOD_THRESHOLD_MV) {
    if (state_ != BatteryState::Good) {
      state_ = BatteryState::Good;
      green_led_.activate();
      red_led_.deactivate();
    }
  } else if (vbatt_mV_ > BATTERY_LOW_THRESHOLD_MV) {
    if (state_ != BatteryState::Low) {
      state_ = BatteryState::Low;
      green_led_.deactivate();
      red_led_.activate();
    }
  } else {
    if (state_ != BatteryState::CriticallyLow) {
      state_ = BatteryState::CriticallyLow;
      green_led_.deactivate();
      red_led_.deactivate();
    }
    red_led_.toggle();
  }
}
