#pragma once
#include <deque>
#include "data_buffer.h"
#include "driver/gpio.h"
#include "gpio.h"
#include "inversion_measuring.h"
#include "rgb_led.h"
#include "shared.h"
#include "task.h"

enum class State {
  kIdle,
  kActive,
  kActuated,
  kCalibrating,
};

class StateMachineTask : public Task {
 public:
  struct Param {
    shared::IMUDataBuffer* imu_data_buffer;
  };

  StateMachineTask(const Task::Config& config, Param* const param)
      : Task(config, param),
        trigger_(PIN_NUM_ACTUATOR, true),
        inversion_measuring_(InversionMeasuring()) {}

 private:
  static constexpr size_t IMU_DATA_HISTORY_LEN = 5;
  static constexpr float IDLE_ACCEL_VARIANCE_THRESHOLD = 1.2;
  static constexpr int IDLE_STATE_TRANSITION_TIME_MS = 1000;
  static constexpr float ACTIVE_INVERSION_SPEED_THRESHOLD_DEG_S = 400;
  static constexpr int ACTUATED_STATE_TIMEOUT_MS = 250;
  static constexpr float INVERSION_SPEED_FILTER_ALPHA = 0.3;
  static constexpr float ACCEL_VARIANCE_FILTER_ALPHA = 0.95;
  static constexpr size_t CALIBRATION_SAMPLES = 3000;
  static constexpr gpio_num_t PIN_NUM_ACTUATOR = GPIO_NUM_1;
  Gpio trigger_;
  InversionMeasuring inversion_measuring_;
  State state_;
  RgbLed rgb_led_;
  std::deque<shared::IMUData> imu_data_history_;
  float filtered_inversion_speed_;
  float filtered_imu_1_accel_variance_;
  float filtered_imu_2_accel_variance_;
  shared::IMUData calibration_data_;
  size_t calibration_sample_counter_;
  size_t idle_counter_;
  size_t actuated_counter_;

  void init();
  void run(void* param);
  void set_status_led(State state);
};