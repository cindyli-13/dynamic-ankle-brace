#include "imu_read_task.h"

void IMUReadTask::init() {
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
      .isr_cpu_id =
          INTR_CPU_ID_AUTO,  // TODO: select proper core if using interrupts
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

  ESP_ERROR_CHECK(imu1.init(imu1_config));
  ESP_ERROR_CHECK(imu2.init(imu2_config));
}

void IMUReadTask::run(void* param) {
  Param* task_param = static_cast<Param*>(param);
  DataBuffer<IMUData>* imu_data_buffer = task_param->imu_data_buffer;

  ICM20948::RawImuData raw_imu1_data;
  ICM20948::RawImuData raw_imu2_data;
  IMUData imu_data_scaled;

  // Poll IMUs at 100Hz
  // TODO: implement interrupts so we can read IMUs at 1kHz
  while (true) {
    ESP_ERROR_CHECK(imu1.read(raw_imu1_data));
    ESP_ERROR_CHECK(imu2.read(raw_imu2_data));

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

    if (!imu_data_buffer->write(imu_data_scaled)) {
      printf("IMU data update failed\n");  // TODO: better error handling
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
