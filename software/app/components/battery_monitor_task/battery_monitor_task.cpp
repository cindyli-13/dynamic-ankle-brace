#include "battery_monitor_task.h"

bool BatteryMonitorTask::adc_callback(adc_continuous_handle_t handle,
                                      const adc_continuous_evt_data_t* edata,
                                      void* user_data) {
  BaseType_t higher_priority_task_woken = pdFALSE;
  xSemaphoreGiveFromISR(static_cast<SemaphoreHandle_t>(user_data),
                        &higher_priority_task_woken);
  return (higher_priority_task_woken == pdTRUE);
}

void BatteryMonitorTask::init() {
  assert(adc_sem_ = xSemaphoreCreateBinaryStatic(&adc_sem_buffer_));

  adc_continuous_handle_cfg_t adc_config = {
      .max_store_buf_size = 16,
      .conv_frame_size = sizeof(adc_result_),
  };
  ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &adc_));

  adc_digi_pattern_config_t adc_pattern = {
      .atten = ADC_ATTENUATION,
      .channel = ADC_CHANNEL,
      .unit = ADC_UNIT,
      .bit_width = ADC_BITWIDTH_DEFAULT,
  };
  adc_continuous_config_t dig_cfg = {
      .pattern_num = 1,
      .adc_pattern = &adc_pattern,
      .sample_freq_hz = 10,
      .conv_mode = ADC_CONV_SINGLE_UNIT_1,
      .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
  };
  ESP_ERROR_CHECK(adc_continuous_config(adc_, &dig_cfg));

  adc_continuous_evt_cbs_t cbs = {
      .on_conv_done = adc_callback,
  };
  ESP_ERROR_CHECK(
      adc_continuous_register_event_callbacks(adc_, &cbs, this->adc_sem_));
  ESP_ERROR_CHECK(adc_continuous_start(adc_));

  green_led_.init();
  red_led_.init();
}

void BatteryMonitorTask::run(void* param) {
  uint32_t adc_result = 0;
  uint32_t adc_num_results = 0;
  while (true) {
    if (!xSemaphoreTake(adc_sem_, portMAX_DELAY)) {
      printf(
          "Failed to acquire ADC semaphore!\n");  // TODO: better error handling
      continue;
    }

    esp_err_t ret = adc_continuous_read(adc_, &adc_result, sizeof(adc_result_),
                                        &adc_num_results, 0);
    if (ret == ESP_OK) {
      adc_digi_output_data_t* adc_output_data =
          reinterpret_cast<adc_digi_output_data_t*>(&adc_result);
      uint32_t adc_data = adc_output_data->type2.data;

      // TODO: do something with the data
    }

    vTaskDelay(1);
  }

  ESP_ERROR_CHECK(adc_continuous_stop(adc_));
  ESP_ERROR_CHECK(adc_continuous_deinit(adc_));
}
