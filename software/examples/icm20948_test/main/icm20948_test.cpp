#include "icm20948.h"
#include <inttypes.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#define SPI_HOST SPI2_HOST

#define PIN_NUM_MISO 13
#define PIN_NUM_MOSI 11
#define PIN_NUM_CLK 12
#define PIN_NUM_CS 10

static constexpr ICM20948::AccelFSR ACCEL_FSR = ICM20948::AccelFSR::_8G;
static constexpr ICM20948::GyroFSR GYRO_FSR = ICM20948::GyroFSR::_1000DPS;

static constexpr ICM20948::Config IMU_CONFIG = {
    .cs_pin = PIN_NUM_CS,
    .spi_clock_speed_hz = 100000,
    .sample_rate_hz = 1000,
    .accel_fsr = ACCEL_FSR,
    .gyro_fsr = GYRO_FSR,
    .accel_dlpf = ICM20948::AccelDLPF::_246HZ,
    .gyro_dlpf = ICM20948::GyroDLPF::_196_6HZ};

extern "C" {

void app_main(void) {
  printf("ICM 20948 Test\n");

  spi_bus_config_t buscfg = {.mosi_io_num = PIN_NUM_MOSI,
                             .miso_io_num = PIN_NUM_MISO,
                             .sclk_io_num = PIN_NUM_CLK,
                             .quadwp_io_num = -1,
                             .quadhd_io_num = -1,
                             .data4_io_num = -1,
                             .data5_io_num = -1,
                             .data6_io_num = -1,
                             .data7_io_num = -1,
                             .max_transfer_sz = 4092,
                             .flags = 0,
                             .isr_cpu_id = INTR_CPU_ID_AUTO,
                             .intr_flags = 0};

  ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

  ICM20948 imu(SPI_HOST);
  ESP_ERROR_CHECK(imu.init(IMU_CONFIG));

  ICM20948::RawImuData imu_data;

  while (true) {
    ESP_ERROR_CHECK(imu.read(imu_data));
    printf(
        "accel_x %.3f accel_y %.3f accel_z %.3f gyro_x %.3f gyro_y %.3f gyro_z "
        "%.3f\n",
        ICM20948::ACCEL_RAW_TO_G(imu_data.accel.x, ACCEL_FSR),
        ICM20948::ACCEL_RAW_TO_G(imu_data.accel.y, ACCEL_FSR),
        ICM20948::ACCEL_RAW_TO_G(imu_data.accel.z, ACCEL_FSR),
        ICM20948::GYRO_RAW_TO_DPS(imu_data.gyro.x, GYRO_FSR),
        ICM20948::GYRO_RAW_TO_DPS(imu_data.gyro.y, GYRO_FSR),
        ICM20948::GYRO_RAW_TO_DPS(imu_data.gyro.z, GYRO_FSR));

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
}
