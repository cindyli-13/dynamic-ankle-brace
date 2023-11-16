#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mpu9250.h"

#define SPI_HOST     SPI2_HOST

#define PIN_NUM_MISO 13
#define PIN_NUM_MOSI 11
#define PIN_NUM_CLK  12
#define PIN_NUM_CS   10

static constexpr MPU9250::AccelFSR ACCEL_FSR = MPU9250::AccelFSR::_8G;
static constexpr MPU9250::GyroFSR GYRO_FSR = MPU9250::GyroFSR::_1000DPS;

static constexpr MPU9250::Config IMU_CONFIG = {
    .cs_pin = PIN_NUM_CS,
    .spi_clock_speed_hz = 100000,
    .sample_rate_hz = 1000,
    .accel_fsr = ACCEL_FSR,
    .gyro_fsr = GYRO_FSR,
    .accel_dlpf = MPU9250::AccelDLPF::_218_1HZ,
    .gyro_dlpf = MPU9250::GyroDLPF::_184HZ
};

extern "C" {

void app_main(void) {
    printf("MPU 9250 Test\n");

    esp_err_t ret;
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
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
        .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
        .intr_flags = 0
    };

    ret = spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    MPU9250 imu(SPI_HOST);
    imu.init(IMU_CONFIG);

    MPU9250::RawImuData imu_data;

    while (true) {
        imu.read(imu_data);
        printf("accel_x %.3f accel_y %.3f accel_z %.3f gyro_x %.3f gyro_y %.3f gyro_z %.3f temp %.3f\n", 
                MPU9250::ACCEL_RAW_TO_G(imu_data.accel.x, ACCEL_FSR),
                MPU9250::ACCEL_RAW_TO_G(imu_data.accel.y, ACCEL_FSR),
                MPU9250::ACCEL_RAW_TO_G(imu_data.accel.z, ACCEL_FSR),
                MPU9250::GYRO_RAW_TO_DPS(imu_data.gyro.x, GYRO_FSR),
                MPU9250::GYRO_RAW_TO_DPS(imu_data.gyro.y, GYRO_FSR),
                MPU9250::GYRO_RAW_TO_DPS(imu_data.gyro.z, GYRO_FSR),
                MPU9250::TEMPERAURE_RAW_TO_DEG_C(imu_data.temperature));

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

}
