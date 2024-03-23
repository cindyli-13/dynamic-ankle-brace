#include "state_machine_task.h"

void StateMachineTask::init() {
  state_ = shared::State::kCalibrating;
  idle_counter_ = 0;
  actuated_counter_ = 0;
  calibration_sample_counter_ = 0;
  calibration_data_ = {0};
  trigger_.init();
  trigger_.deactivate();
  rgb_led_.init();
}

void StateMachineTask::run(void* param) {
  while (true) {
    Param* task_param = static_cast<Param*>(param);

    shared::IMUData imu_data;
    while (task_param->imu_data_buffer->receive(imu_data)) {
      imu_data_history_.push_back(imu_data);
      if (imu_data_history_.size() > IMU_DATA_HISTORY_LEN) {
        imu_data_history_.pop_front();
      }
      if (state_ == shared::State::kCalibrating) {
        calibration_data_.imu1.accel.x += imu_data.imu1.accel.x;
        calibration_data_.imu1.accel.y += imu_data.imu1.accel.y;
        calibration_data_.imu1.accel.z += imu_data.imu1.accel.z;
        calibration_data_.imu2.accel.x += imu_data.imu2.accel.x;
        calibration_data_.imu2.accel.y += imu_data.imu2.accel.y;
        calibration_data_.imu2.accel.z += imu_data.imu2.accel.z;
        calibration_sample_counter_++;
      } else {
        inversion_speed_filter_.update(
            inversion_measuring_.get_inversion_speed(imu_data));
        imu_1_accel_variance_filter_.update(
            imu_data.imu1.accel.x * imu_data.imu1.accel.x +
            imu_data.imu1.accel.y * imu_data.imu1.accel.y +
            imu_data.imu1.accel.z * imu_data.imu1.accel.z);
        imu_2_accel_variance_filter_.update(
            imu_data.imu2.accel.x * imu_data.imu2.accel.x +
            imu_data.imu2.accel.y * imu_data.imu2.accel.y +
            imu_data.imu2.accel.z * imu_data.imu2.accel.z);
      }
    }

    shared::Config requested_config;
    if (task_param->config_params_buffer->receive(requested_config)) {
      config_ = requested_config;
    }

    bool calibration_request = false;
    // state transition logic
    switch (state_) {
      case shared::State::kCalibrating:
        if (calibration_sample_counter_ > CALIBRATION_SAMPLES) {
          inversion_measuring_.calibrate(
              calibration_data_.imu1.accel.x / CALIBRATION_SAMPLES,
              calibration_data_.imu1.accel.y / CALIBRATION_SAMPLES,
              calibration_data_.imu1.accel.z / CALIBRATION_SAMPLES,
              calibration_data_.imu2.accel.x / CALIBRATION_SAMPLES,
              calibration_data_.imu2.accel.y / CALIBRATION_SAMPLES,
              calibration_data_.imu2.accel.z / CALIBRATION_SAMPLES);
          calibration_data_ = {0};
          calibration_sample_counter_ = 0;
          state_ = shared::State::kIdle;
        }
        break;
      case shared::State::kIdle:
        if (imu_1_accel_variance_filter_.get() >
                config_.idle_variance_threshold &&
            imu_2_accel_variance_filter_.get() >
                config_.idle_variance_threshold) {
          idle_counter_ = 0;
          state_ = shared::State::kActive;
        }
        if (task_param->calibration_requested_buffer->receive(
                calibration_request) &&
            calibration_request) {
          state_ = shared::State::kCalibrating;
        }
        break;
      case shared::State::kActive:
        if (inversion_speed_filter_.get() > config_.inversion_threshold_deg_s) {
          trigger_.activate();
          state_ = shared::State::kActuated;
        } else if (imu_1_accel_variance_filter_.get() <
                       config_.idle_variance_threshold &&
                   imu_2_accel_variance_filter_.get() <
                       config_.idle_variance_threshold) {
          idle_counter_++;
          if (idle_counter_ >
              config_.idle_transition_time_ms) {  // 1 kHz loop -> 1ms/loop
            state_ = shared::State::kIdle;
            idle_counter_ = 0;
          }
        } else {
          idle_counter_ = 0;
        }
        if (task_param->calibration_requested_buffer->receive(
                calibration_request) &&
            calibration_request) {
          state_ = shared::State::kCalibrating;
        }
        break;
      case shared::State::kActuated:
        actuated_counter_++;
        if (actuated_counter_ >
            config_.actuation_timeout_ms) {  // 1 kHz loop -> 1ms/loop
          trigger_.deactivate();
          actuated_counter_ = 0;
          state_ = shared::State::kActive;
        }
        break;
    }
    // update data for telemetry
    float inversion_speed = inversion_speed_filter_.get();
    task_param->inversion_speed_buffer->send(inversion_speed);
    task_param->state_buffer->send(state_);
    set_status_led(state_);

    vTaskDelay(1 / portTICK_PERIOD_MS);  // run at 1 kHz
  }
}

void StateMachineTask::set_status_led(shared::State state) {
  switch (state) {
    case shared::State::kIdle:
      rgb_led_.set(0, 0, 16);  // blue
      break;
    case shared::State::kActive:
      rgb_led_.set(0, 16, 0);  // green
      break;
    case shared::State::kActuated:
      rgb_led_.set(16, 0, 0);  // red
      break;
    case shared::State::kCalibrating:
      rgb_led_.set(16, 0, 16);  // purple
      break;
  }
}