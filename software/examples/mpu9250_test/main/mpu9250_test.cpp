#include "mpu9250.h"
#include <inttypes.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#define SPI_HOST SPI2_HOST

#define PIN_NUM_MISO 13
#define PIN_NUM_MOSI 11
#define PIN_NUM_CLK 12
#define PIN_NUM_CS1 10  // IMU 1
#define PIN_NUM_CS2 14  // IMU 2

static constexpr MPU9250::AccelFSR ACCEL_FSR = MPU9250::AccelFSR::_8G;
static constexpr MPU9250::GyroFSR GYRO_FSR = MPU9250::GyroFSR::_1000DPS;

static constexpr MPU9250::Config IMU1_CONFIG = {
    .cs_pin = PIN_NUM_CS1,
    .spi_clock_speed_hz = 100000,
    .sample_rate_hz = 1000,
    .accel_fsr = ACCEL_FSR,
    .gyro_fsr = GYRO_FSR,
    .accel_dlpf = MPU9250::AccelDLPF::_218_1HZ,
    .gyro_dlpf = MPU9250::GyroDLPF::_184HZ};

static constexpr MPU9250::Config IMU2_CONFIG = {
    .cs_pin = PIN_NUM_CS2,
    .spi_clock_speed_hz = 100000,
    .sample_rate_hz = 1000,
    .accel_fsr = ACCEL_FSR,
    .gyro_fsr = GYRO_FSR,
    .accel_dlpf = MPU9250::AccelDLPF::_218_1HZ,
    .gyro_dlpf = MPU9250::GyroDLPF::_184HZ};

extern "C" {

void app_main(void) {
  printf("MPU 9250 Test\n");

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

  MPU9250 imu1(SPI_HOST), imu2(SPI_HOST);
  ESP_ERROR_CHECK(imu1.init(IMU1_CONFIG));
  ESP_ERROR_CHECK(imu2.init(IMU2_CONFIG));

  MPU9250::RawImuData imu1_data, imu2_data;

  while (true) {
    ESP_ERROR_CHECK(imu1.read(imu1_data));
    ESP_ERROR_CHECK(imu2.read(imu2_data));
    printf(
        "accel_x %.3f accel_y %.3f accel_z %.3f gyro_x %.3f gyro_y %.3f gyro_z "
        "%.3f temp %.3f\n",
        MPU9250::ACCEL_RAW_TO_G(imu1_data.accel.x - imu2_data.accel.x,
                                ACCEL_FSR),
        MPU9250::ACCEL_RAW_TO_G(imu1_data.accel.y - imu2_data.accel.y,
                                ACCEL_FSR),
        MPU9250::ACCEL_RAW_TO_G(imu1_data.accel.z - imu2_data.accel.z,
                                ACCEL_FSR),
        MPU9250::GYRO_RAW_TO_DPS(imu1_data.gyro.x - imu2_data.gyro.x, GYRO_FSR),
        MPU9250::GYRO_RAW_TO_DPS(imu1_data.gyro.y - imu2_data.gyro.y, GYRO_FSR),
        MPU9250::GYRO_RAW_TO_DPS(imu1_data.gyro.z - imu2_data.gyro.z, GYRO_FSR),
        MPU9250::TEMPERAURE_RAW_TO_DEG_C(imu1_data.temperature -
                                         imu2_data.temperature));

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
}
