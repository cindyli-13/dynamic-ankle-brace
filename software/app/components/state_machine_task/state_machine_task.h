#pragma once
#include <deque>
#include "data_buffer.h"
#include "driver/gpio.h"
#include "ema_filter.h"
#include "esp_timer.h"
#include "gpio.h"
#include "inversion_measuring.h"
#include "rgb_led.h"
#include "shared.h"
#include "task.h"

static constexpr float INVERSION_SPEED_FILTER_ALPHA = 0.3;
static constexpr float ACCEL_VARIANCE_FILTER_ALPHA = 0.95;

class StateMachineTask : public Task {
 public:
  struct Param {
    shared::IMUDataBuffer* imu_data_buffer;
    DataBuffer<shared::StampedInversionSpeed, 20>* inversion_speed_buffer;
    DataBuffer<shared::State, 1>* state_buffer;
    DataBuffer<bool, 1>* calibration_requested_buffer;
    DataBuffer<shared::Config, 1>* config_params_buffer;
    DataBuffer<shared::Config, 1>* internal_config_buffer;
  };

  StateMachineTask(const Task::Config& config, Param* const param)
      : Task(config, param),
        trigger_(PIN_NUM_ACTUATOR, true),
        inversion_measuring_(InversionMeasuring()),
        inversion_speed_filter_(INVERSION_SPEED_FILTER_ALPHA),
        imu_1_accel_variance_filter_(ACCEL_VARIANCE_FILTER_ALPHA),
        imu_2_accel_variance_filter_(ACCEL_VARIANCE_FILTER_ALPHA) {}

 private:
  static constexpr size_t IMU_DATA_HISTORY_LEN = 5;
  static constexpr size_t CALIBRATION_SAMPLES = 3000;
  static constexpr gpio_num_t PIN_NUM_ACTUATOR = GPIO_NUM_1;
  shared::Config config_ = {.inversion_threshold_deg_s = 300,
                            .actuation_timeout_ms = 2500,
                            .idle_variance_threshold = 1.2,
                            .idle_transition_time_ms = 60000};
  Gpio trigger_;
  InversionMeasuring inversion_measuring_;
  shared::State state_;
  RgbLed rgb_led_;
  std::deque<shared::IMUData> imu_data_history_;
  EMAFilter inversion_speed_filter_;
  EMAFilter imu_1_accel_variance_filter_;
  EMAFilter imu_2_accel_variance_filter_;
  shared::IMUData calibration_data_;
  size_t calibration_sample_counter_;
  size_t idle_counter_;
  size_t actuated_counter_;

  void init();
  void run(void* param);
  void set_status_led(shared::State state);
};