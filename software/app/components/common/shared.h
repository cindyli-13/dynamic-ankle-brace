#pragma once

#include "config.h"
#include "data_buffer.h"

namespace shared {

// Gyro data in deg/s, accelerometer data in g's
struct IMUData {
  struct {
    struct {
      float x;
      float y;
      float z;
    } gyro;
    struct {
      float x;
      float y;
      float z;
    } accel;
  } imu1;
  struct {
    struct {
      float x;
      float y;
      float z;
    } gyro;
    struct {
      float x;
      float y;
      float z;
    } accel;
  } imu2;
};

typedef DataBuffer<IMUData, config::IMU_DATA_BUFFER_LEN> IMUDataBuffer;

struct Config {
  float inversion_threshold_deg_s;
  int actuation_timeout_ms;
  float idle_variance_threshold;
  int idle_transition_time_ms;
};

enum class State {
  kIdle,
  kActive,
  kActuated,
  kCalibrating,
};

}  // namespace shared
