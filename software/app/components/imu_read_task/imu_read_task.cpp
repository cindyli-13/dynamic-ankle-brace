#include "imu_read_task.h"

void IMUReadTask::interrupt_handler(void* arg) {
  BaseType_t higher_priority_task_woken = pdFALSE;
  xSemaphoreGiveFromISR(static_cast<SemaphoreHandle_t>(arg),
                        &higher_priority_task_woken);
  portYIELD_FROM_ISR(higher_priority_task_woken);
}

void IMUReadTask::init() {
  assert(int1_sem_ = xSemaphoreCreateBinaryStatic(&int1_sem_buffer_));
  assert(int2_sem_ = xSemaphoreCreateBinaryStatic(&int2_sem_buffer_));

  // Configure INT1 interrupts for both IMUs
  gpio_config_t int1_cfg = {.pin_bit_mask = (1ULL << PIN_NUM_INT1),
                            .mode = GPIO_MODE_INPUT,
                            .pull_up_en = GPIO_PULLUP_DISABLE,
                            .pull_down_en = GPIO_PULLDOWN_ENABLE,
                            .intr_type = GPIO_INTR_POSEDGE};

  gpio_config_t int2_cfg = {.pin_bit_mask = (1ULL << PIN_NUM_INT2),
                            .mode = GPIO_MODE_INPUT,
                            .pull_up_en = GPIO_PULLUP_DISABLE,
                            .pull_down_en = GPIO_PULLDOWN_ENABLE,
                            .intr_type = GPIO_INTR_POSEDGE};

  ESP_ERROR_CHECK(gpio_config(&int1_cfg));
  ESP_ERROR_CHECK(gpio_config(&int2_cfg));
  ESP_ERROR_CHECK(gpio_install_isr_service(0));
  ESP_ERROR_CHECK(
      gpio_isr_handler_add(PIN_NUM_INT1, interrupt_handler, int1_sem_));
  ESP_ERROR_CHECK(
      gpio_isr_handler_add(PIN_NUM_INT2, interrupt_handler, int2_sem_));

  // First need to convert core_id to intr_cpu_id_t enum type
  intr_cpu_id_t core_id;
  switch (core_id_) {
    case 0:
      core_id = INTR_CPU_ID_0;
      break;
    case 1:
      core_id = INTR_CPU_ID_1;
      break;
    default:
      core_id = INTR_CPU_ID_AUTO;
      break;
  };

  // Configure SPI bus
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
                             .isr_cpu_id = core_id,
                             .intr_flags = 0};

  ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

  ICM20948::Config imu1_config = {.cs_pin = PIN_NUM_CS1,
                                  .spi_clock_speed_hz = 100000,
                                  .sample_rate_hz = 1000,
                                  .accel_fsr = ACCEL_FSR,
                                  .gyro_fsr = GYRO_FSR,
                                  .accel_dlpf = ICM20948::AccelDLPF::_246HZ,
                                  .gyro_dlpf = ICM20948::GyroDLPF::_196_6HZ};

  ICM20948::Config imu2_config = {.cs_pin = PIN_NUM_CS2,
                                  .spi_clock_speed_hz = 100000,
                                  .sample_rate_hz = 1000,
                                  .accel_fsr = ACCEL_FSR,
                                  .gyro_fsr = GYRO_FSR,
                                  .accel_dlpf = ICM20948::AccelDLPF::_246HZ,
                                  .gyro_dlpf = ICM20948::GyroDLPF::_196_6HZ};

  ESP_ERROR_CHECK(imu2_.init(imu2_config));
  ESP_ERROR_CHECK(imu1_.init(imu1_config));
}

void IMUReadTask::run(void* param) {
  Param* task_param = static_cast<Param*>(param);
  shared::IMUDataBuffer* imu_data_buffer = task_param->imu_data_buffer;

  ICM20948::RawImuData raw_imu1_data;
  ICM20948::RawImuData raw_imu2_data;
  shared::IMUData imu_data_scaled;

  while (true) {
    if (!xSemaphoreTake(int1_sem_, portMAX_DELAY)) {
      printf(
          "Failed to acquire IMU1 semaphore!\n");  // TODO: better error handling
      continue;
    }

    // Read IMU1 data
    ESP_ERROR_CHECK(imu1_.read(raw_imu1_data));
    imu_data_scaled.imu1.accel.x =
        ICM20948::ACCEL_RAW_TO_G(raw_imu1_data.accel.x, ACCEL_FSR);
    imu_data_scaled.imu1.accel.y =
        ICM20948::ACCEL_RAW_TO_G(raw_imu1_data.accel.y, ACCEL_FSR);
    imu_data_scaled.imu1.accel.z =
        ICM20948::ACCEL_RAW_TO_G(raw_imu1_data.accel.z, ACCEL_FSR);
    imu_data_scaled.imu1.gyro.x =
        ICM20948::GYRO_RAW_TO_DPS(raw_imu1_data.gyro.x, GYRO_FSR);
    imu_data_scaled.imu1.gyro.y =
        ICM20948::GYRO_RAW_TO_DPS(raw_imu1_data.gyro.y, GYRO_FSR);
    imu_data_scaled.imu1.gyro.z =
        ICM20948::GYRO_RAW_TO_DPS(raw_imu1_data.gyro.z, GYRO_FSR);

    if (!xSemaphoreTake(int2_sem_, portMAX_DELAY)) {
      printf(
          "Failed to acquire IMU2 semaphore!\n");  // TODO: better error handling
      continue;
    }

    // Read IMU2 data
    ESP_ERROR_CHECK(imu2_.read(raw_imu2_data));
    imu_data_scaled.imu2.accel.x =
        ICM20948::ACCEL_RAW_TO_G(raw_imu2_data.accel.x, ACCEL_FSR);
    imu_data_scaled.imu2.accel.y =
        ICM20948::ACCEL_RAW_TO_G(raw_imu2_data.accel.y, ACCEL_FSR);
    imu_data_scaled.imu2.accel.z =
        ICM20948::ACCEL_RAW_TO_G(raw_imu2_data.accel.z, ACCEL_FSR);
    imu_data_scaled.imu2.gyro.x =
        ICM20948::GYRO_RAW_TO_DPS(raw_imu2_data.gyro.x, GYRO_FSR);
    imu_data_scaled.imu2.gyro.y =
        ICM20948::GYRO_RAW_TO_DPS(raw_imu2_data.gyro.y, GYRO_FSR);
    imu_data_scaled.imu2.gyro.z =
        ICM20948::GYRO_RAW_TO_DPS(raw_imu2_data.gyro.z, GYRO_FSR);

    // Update IMU data buffer with new data
    if (!imu_data_buffer->send(imu_data_scaled)) {
      printf("IMU data update failed\n");  // TODO: better error handling
    }
  }
}
