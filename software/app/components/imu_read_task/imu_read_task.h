#pragma once

#include <inttypes.h>
#include <stdio.h>
#include "data_buffer.h"
#include "icm20948.h"
#include "task.h"

class IMUReadTask : public Task {
 public:
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

  struct Param {
    DataBuffer<IMUData>* imu_data_buffer;
  };

  IMUReadTask(const Task::Config& config, Param* const param)
      : Task(config, param), imu1(SPI_HOST), imu2(SPI_HOST) {}

 private:
  static constexpr spi_host_device_t SPI_HOST = SPI2_HOST;
  static constexpr int PIN_NUM_MISO = 13;
  static constexpr int PIN_NUM_MOSI = 11;
  static constexpr int PIN_NUM_CLK = 12;
  static constexpr int PIN_NUM_INT1 = 9;   // IMU 1
  static constexpr int PIN_NUM_INT2 = 46;  // IMU 2
  static constexpr int PIN_NUM_CS1 = 10;   // IMU 1
  static constexpr int PIN_NUM_CS2 = 14;   // IMU 2

  static constexpr ICM20948::AccelFSR ACCEL_FSR = ICM20948::AccelFSR::_8G;
  static constexpr ICM20948::GyroFSR GYRO_FSR = ICM20948::GyroFSR::_1000DPS;

  ICM20948 imu1;
  ICM20948 imu2;

  void init();
  void run(void* param);
};
