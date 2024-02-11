#include <inttypes.h>
#include <stdio.h>
#include <eigen3/Eigen/Eigen>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mpu9250.h"
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
  printf("Inversion Measuring Test\n");

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

  // Calibration Procedure
  // Make sure the IMUs are kept very still. This procedure will sample the measured gravity vectors from the two
  // IMUs and take the normalized readings as imu1_g and imu2_g. Then compute rotation matrix R that transforms from
  // IMU 2's frame to IMU 1. Note that IMU 1 is mounted to the lower leg and IMU 2 is mounted to the foot.
  // Source: https://math.stackexchange.com/questions/180418/calculate-rotation-matrix-to-align-vector-a-to-vector-b-in-3d/
  printf(
      "Calibrating... please keep the IMUs very still in the rest position\n");
  vTaskDelay(2000 / portTICK_PERIOD_MS);

  // Collect measurements for 3 seconds
  float imu1_g_x = 0, imu1_g_y = 0, imu1_g_z = 0, imu2_g_x = 0, imu2_g_y = 0,
        imu2_g_z = 0;
  for (int i = 0; i < 300; i++) {
    ESP_ERROR_CHECK(imu1.read(imu1_data));
    ESP_ERROR_CHECK(imu2.read(imu2_data));

    imu1_g_x += imu1_data.accel.x;
    imu1_g_y += imu1_data.accel.y;
    imu1_g_z += imu1_data.accel.z;
    imu2_g_x += imu2_data.accel.x;
    imu2_g_y += imu2_data.accel.y;
    imu2_g_z += imu2_data.accel.z;

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
  Eigen::Vector3f imu1_g{imu1_g_x, imu1_g_y, imu1_g_z};
  Eigen::Vector3f imu2_g{imu2_g_x, imu2_g_y, imu2_g_z};
  imu1_g.normalize();
  imu2_g.normalize();

  Eigen::Vector3f v = imu2_g.cross(imu1_g);
  float c = imu2_g.dot(imu1_g);
  Eigen::Matrix3f w{{0, -v(2), v(1)},
                    {v(2), 0, -v(0)},
                    {-v(1), v(0), 0}};  //skew-symmetric matrix
  Eigen::Matrix3f R =
      Eigen::Matrix3f::Identity() + w +
      w * w * (1 / (1 + c));  // rotation matrix from {imu2} to {imu1}
  printf("Calibration complete\n");

  while (true) {
    ESP_ERROR_CHECK(imu1.read(imu1_data));
    ESP_ERROR_CHECK(imu2.read(imu2_data));

    Eigen::Vector3f imu1_gyro_scaled{
        MPU9250::GYRO_RAW_TO_DPS(imu1_data.gyro.x, GYRO_FSR),
        MPU9250::GYRO_RAW_TO_DPS(imu1_data.gyro.y, GYRO_FSR),
        MPU9250::GYRO_RAW_TO_DPS(imu1_data.gyro.z, GYRO_FSR)};

    Eigen::Vector3f imu2_gyro_scaled{
        MPU9250::GYRO_RAW_TO_DPS(imu2_data.gyro.x, GYRO_FSR),
        MPU9250::GYRO_RAW_TO_DPS(imu2_data.gyro.y, GYRO_FSR),
        MPU9250::GYRO_RAW_TO_DPS(imu2_data.gyro.z, GYRO_FSR)};

    Eigen::Vector3f relative_angular_vel =
        R * imu2_gyro_scaled - imu1_gyro_scaled;

    printf("inversion_speed_dps %.3f\n", relative_angular_vel(1));

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
}
