#include "icm20948.h"
#include <inttypes.h>
#include <stdio.h>
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#define USE_POLLING 0  // 0 for interrupt, 1 for polling

static constexpr spi_host_device_t SPI_HOST = SPI2_HOST;

static constexpr gpio_num_t PIN_NUM_MISO = GPIO_NUM_13;
static constexpr gpio_num_t PIN_NUM_MOSI = GPIO_NUM_11;
static constexpr gpio_num_t PIN_NUM_CLK = GPIO_NUM_12;
static constexpr gpio_num_t PIN_NUM_CS = GPIO_NUM_10;
static constexpr gpio_num_t PIN_INT1 = GPIO_NUM_9;

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

#if USE_POLLING == 0
static SemaphoreHandle_t interrupt_flag;
static StaticSemaphore_t interrupt_flag_buffer;

void interrupt_handler(void* arg) {
  BaseType_t higher_priority_task_woken = pdFALSE;
  xSemaphoreGiveFromISR(interrupt_flag, &higher_priority_task_woken);
  portYIELD_FROM_ISR(higher_priority_task_woken);
}
#endif

extern "C" {

void app_main(void) {
  printf("ICM 20948 Test\n");

#if USE_POLLING == 0
  gpio_config_t int1_cfg = {.pin_bit_mask = (1ULL << PIN_INT1),
                            .mode = GPIO_MODE_INPUT,
                            .pull_up_en = GPIO_PULLUP_DISABLE,
                            .pull_down_en = GPIO_PULLDOWN_ENABLE,
                            .intr_type = GPIO_INTR_POSEDGE};
  ESP_ERROR_CHECK(gpio_config(&int1_cfg));
  ESP_ERROR_CHECK(gpio_install_isr_service(0));
  ESP_ERROR_CHECK(gpio_isr_handler_add(PIN_INT1, interrupt_handler, nullptr));

  assert(interrupt_flag = xSemaphoreCreateBinaryStatic(&interrupt_flag_buffer));
#endif

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

  int64_t prev_time_us = esp_timer_get_time();
  int counter = 0;

  while (true) {
#if USE_POLLING == 0
    if (!xSemaphoreTake(interrupt_flag, portMAX_DELAY)) {
      continue;
    }
#endif
    ESP_ERROR_CHECK(imu.read(imu_data));

    int64_t curr_time_us = esp_timer_get_time();
    if (counter % 10 == 0) {
      printf(
          "accel_x %.3f accel_y %.3f accel_z %.3f gyro_x %.3f gyro_y %.3f "
          "gyro_z "
          "%.3f sample_time_ms %.3f\n",
          ICM20948::ACCEL_RAW_TO_G(imu_data.accel.x, ACCEL_FSR),
          ICM20948::ACCEL_RAW_TO_G(imu_data.accel.y, ACCEL_FSR),
          ICM20948::ACCEL_RAW_TO_G(imu_data.accel.z, ACCEL_FSR),
          ICM20948::GYRO_RAW_TO_DPS(imu_data.gyro.x, GYRO_FSR),
          ICM20948::GYRO_RAW_TO_DPS(imu_data.gyro.y, GYRO_FSR),
          ICM20948::GYRO_RAW_TO_DPS(imu_data.gyro.z, GYRO_FSR),
          (curr_time_us - prev_time_us) * 0.001f);
    }
    prev_time_us = curr_time_us;

#if USE_POLLING == 1
    vTaskDelay(10 / portTICK_PERIOD_MS);
#endif
    counter++;
  }
}
}
