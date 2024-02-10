#pragma once

#include <inttypes.h>
#include <stdio.h>
#include "data_buffer.h"
#include "driver/gpio.h"
#include "freertos/semphr.h"
#include "icm20948.h"
#include "shared.h"
#include "task.h"

// Task for reading data from IMUs
class IMUReadTask : public Task {
 public:
  struct Param {
    shared::IMUDataBuffer* imu_data_buffer;
  };

  IMUReadTask(const Task::Config& config, Param* const param)
      : Task(config, param),
        imu1_(SPI_HOST),
        imu2_(SPI_HOST),
        core_id_(config.core_id) {}

 private:
  static void IRAM_ATTR interrupt_handler(void* arg);

  static constexpr spi_host_device_t SPI_HOST = SPI2_HOST;
  static constexpr gpio_num_t PIN_NUM_MISO = GPIO_NUM_13;
  static constexpr gpio_num_t PIN_NUM_MOSI = GPIO_NUM_11;
  static constexpr gpio_num_t PIN_NUM_CLK = GPIO_NUM_12;
  static constexpr gpio_num_t PIN_NUM_INT1 = GPIO_NUM_9;   // IMU 1
  static constexpr gpio_num_t PIN_NUM_INT2 = GPIO_NUM_46;  // IMU 2
  static constexpr gpio_num_t PIN_NUM_CS1 = GPIO_NUM_10;   // IMU 1
  static constexpr gpio_num_t PIN_NUM_CS2 = GPIO_NUM_14;   // IMU 2

  static constexpr ICM20948::AccelFSR ACCEL_FSR = ICM20948::AccelFSR::_8G;
  static constexpr ICM20948::GyroFSR GYRO_FSR = ICM20948::GyroFSR::_1000DPS;

  ICM20948 imu1_;
  ICM20948 imu2_;

  SemaphoreHandle_t int1_sem_;
  SemaphoreHandle_t int2_sem_;
  StaticSemaphore_t int1_sem_buffer_;
  StaticSemaphore_t int2_sem_buffer_;

  const BaseType_t core_id_;

  void init();
  void run(void* param);
};
