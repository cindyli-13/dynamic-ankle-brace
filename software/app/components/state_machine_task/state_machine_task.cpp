#include "state_machine_task.h"

void StateMachineTask::init() {
  state_ = State::kCalibrating;
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
    shared::IMUDataBuffer* imu_data_buffer = task_param->imu_data_buffer;

    shared::IMUData imu_data;
    while (imu_data_buffer->receive(imu_data)) {
      imu_data_history_.push_back(imu_data);
      if (imu_data_history_.size() > IMU_DATA_HISTORY_LEN) {
        imu_data_history_.pop_front();
      }
      if (state_ == State::kCalibrating) {
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

    // state transition logic
    switch (state_) {
      case State::kCalibrating:
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
          state_ = State::kIdle;
        }
        break;
      case State::kIdle:
        if (imu_1_accel_variance_filter_.get() >
                IDLE_ACCEL_VARIANCE_THRESHOLD &&
            imu_2_accel_variance_filter_.get() >
                IDLE_ACCEL_VARIANCE_THRESHOLD) {
          idle_counter_ = 0;
          state_ = State::kActive;
        }
        break;
      case State::kActive:
        if (inversion_speed_filter_.get() >
            ACTIVE_INVERSION_SPEED_THRESHOLD_DEG_S) {
          trigger_.activate();
          state_ = State::kActuated;
        } else if (imu_1_accel_variance_filter_.get() <
                       IDLE_ACCEL_VARIANCE_THRESHOLD &&
                   imu_2_accel_variance_filter_.get() <
                       IDLE_ACCEL_VARIANCE_THRESHOLD) {
          idle_counter_++;
          if (idle_counter_ >
              IDLE_STATE_TRANSITION_TIME_MS) {  // 1 kHz loop -> 1ms/loop
            state_ = State::kIdle;
            idle_counter_ = 0;
          }
        } else {
          idle_counter_ = 0;
        }
        break;
      case State::kActuated:
        actuated_counter_++;
        if (actuated_counter_ >
            ACTUATED_STATE_TIMEOUT_MS) {  // 1 kHz loop -> 1ms/loop
          trigger_.deactivate();
          actuated_counter_ = 0;
          state_ = State::kActive;
        }
        break;
    }
    set_status_led(state_);

    vTaskDelay(1 / portTICK_PERIOD_MS);  // run at 1 kHz
  }
}

void StateMachineTask::set_status_led(State state) {
  switch (state) {
    case State::kIdle:
      rgb_led_.set(0, 0, 16);  // blue
      break;
    case State::kActive:
      rgb_led_.set(0, 16, 0);  // green
      break;
    case State::kActuated:
      rgb_led_.set(16, 0, 0);  // red
      break;
    case State::kCalibrating:
      rgb_led_.set(16, 0, 16);  // purple
      break;
  }
}