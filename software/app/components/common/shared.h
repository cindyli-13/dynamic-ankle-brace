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

}  // namespace shared
