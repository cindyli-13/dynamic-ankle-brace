#include "mpu9250.h"

// TODO: figure out best way to do error handling
// ESP-IDF has existing error check macros under esp_check.h but none exactly like this one:
#define RETURN_ON_ERR(x) \
  do {                   \
    esp_err_t err = (x); \
    if (err != ESP_OK) { \
      return err;        \
    }                    \
  } while (0)

esp_err_t MPU9250::init(const Config& config) {
  spi_device_interface_config_t devcfg = {
      .command_bits = 0,
      .address_bits = 8,
      .dummy_bits = 0,
      .mode = 3,
      .clock_source = SPI_CLK_SRC_DEFAULT,
      .duty_cycle_pos = 0,
      .cs_ena_pretrans = 0,
      .cs_ena_posttrans = 0,
      .clock_speed_hz = config.spi_clock_speed_hz,
      .input_delay_ns = 0,
      .spics_io_num = config.cs_pin,
      .flags = 0,
      .queue_size = 8,
      .pre_cb = nullptr,
      .post_cb = nullptr};

  RETURN_ON_ERR(spi_bus_add_device(spi_host_, &devcfg, &spi_handle_));

  RETURN_ON_ERR(reset());
  RETURN_ON_ERR(set_sleep(false));
  RETURN_ON_ERR(disable_i2c(false));
  RETURN_ON_ERR(set_clock_source(ClockSource::PLL));

  // Verify device whoami
  uint8_t data;
  RETURN_ON_ERR(whoami(data));
  if (data != DEFAULT_WHOAMI) {
    return ESP_ERR_INVALID_RESPONSE;
  }

  RETURN_ON_ERR(set_fsr(config.accel_fsr, config.gyro_fsr));
  RETURN_ON_ERR(set_dlpf(config.accel_dlpf, config.gyro_dlpf));
  RETURN_ON_ERR(set_sample_rate(config.sample_rate_hz));

  return ESP_OK;
}

esp_err_t MPU9250::read(RawImuData& imu_data) {
  uint8_t buffer[14] = {0};
  RETURN_ON_ERR(read_bytes(Register::ACCEL_XOUT_H, 14, buffer));

  imu_data.accel.x = (static_cast<uint16_t>(buffer[0]) << 8) |
                     static_cast<uint16_t>(buffer[1]);
  imu_data.accel.y = (static_cast<uint16_t>(buffer[2]) << 8) |
                     static_cast<uint16_t>(buffer[3]);
  imu_data.accel.z = (static_cast<uint16_t>(buffer[4]) << 8) |
                     static_cast<uint16_t>(buffer[5]);
  imu_data.temperature = (static_cast<uint16_t>(buffer[6]) << 8) |
                         static_cast<uint16_t>(buffer[7]);
  imu_data.gyro.x = (static_cast<uint16_t>(buffer[8]) << 8) |
                    static_cast<uint16_t>(buffer[9]);
  imu_data.gyro.y = (static_cast<uint16_t>(buffer[10]) << 8) |
                    static_cast<uint16_t>(buffer[11]);
  imu_data.gyro.z = (static_cast<uint16_t>(buffer[12]) << 8) |
                    static_cast<uint16_t>(buffer[13]);

  return ESP_OK;
}

esp_err_t MPU9250::reset() {
  RETURN_ON_ERR(write_bit(Register::PWR_MGMT_1,
                          static_cast<uint8_t>(PwrMgmt1Bits::H_RESET), 1));
  RETURN_ON_ERR(write_bit(Register::USER_CTRL,
                          static_cast<uint8_t>(UserCtrlBits::SIG_COND_RST), 1));
  return ESP_OK;
}

esp_err_t MPU9250::set_sleep(bool sleep) {
  RETURN_ON_ERR(write_bit(Register::PWR_MGMT_1,
                          static_cast<uint8_t>(PwrMgmt1Bits::SLEEP), sleep));
  return ESP_OK;
}

esp_err_t MPU9250::disable_i2c(bool disable) {
  RETURN_ON_ERR(write_bit(Register::USER_CTRL,
                          static_cast<uint8_t>(UserCtrlBits::I2C_IF_DIS),
                          disable));
  return ESP_OK;
}

esp_err_t MPU9250::set_clock_source(ClockSource clock_source) {
  RETURN_ON_ERR(write_bits(Register::PWR_MGMT_1,
                           static_cast<uint8_t>(PwrMgmt1Bits::CLK_SEL_LSB), 3,
                           static_cast<uint8_t>(clock_source)));
  return ESP_OK;
}

esp_err_t MPU9250::whoami(uint8_t& val) {
  RETURN_ON_ERR(read_reg(Register::WHOAMI, val));
  return ESP_OK;
}

esp_err_t MPU9250::set_fsr(AccelFSR accel_fsr, GyroFSR gyro_fsr) {
  RETURN_ON_ERR(write_bits(Register::ACCEL_CONFIG,
                           static_cast<uint8_t>(AccelConfigBits::FS_SEL_LSB), 2,
                           static_cast<uint8_t>(accel_fsr)));
  RETURN_ON_ERR(write_bits(Register::GYRO_CONFIG,
                           static_cast<uint8_t>(GyroConfigBits::FS_SEL_LSB), 2,
                           static_cast<uint8_t>(gyro_fsr)));
  return ESP_OK;
}

esp_err_t MPU9250::set_dlpf(AccelDLPF accel_dlpf, GyroDLPF gyro_dlpf) {
  if (accel_dlpf == AccelDLPF::NONE) {
    RETURN_ON_ERR(write_bit(Register::ACCEL_CONFIG_2,
                            static_cast<uint8_t>(AccelConfig2Bits::F_CHOICE_B),
                            1));
  } else {
    RETURN_ON_ERR(write_bit(Register::ACCEL_CONFIG_2,
                            static_cast<uint8_t>(AccelConfig2Bits::F_CHOICE_B),
                            0));
    RETURN_ON_ERR(
        write_bits(Register::ACCEL_CONFIG_2,
                   static_cast<uint8_t>(AccelConfig2Bits::A_DLPFCFG_LSB), 3,
                   static_cast<uint8_t>(accel_dlpf)));
  }

  if (gyro_dlpf == GyroDLPF::NONE) {
    RETURN_ON_ERR(write_bits(
        Register::GYRO_CONFIG,
        static_cast<uint8_t>(GyroConfigBits::F_CHOICE_B_LSB), 2, 0b11));
  } else {
    RETURN_ON_ERR(write_bits(
        Register::GYRO_CONFIG,
        static_cast<uint8_t>(GyroConfigBits::F_CHOICE_B_LSB), 2, 0b00));
    RETURN_ON_ERR(write_bits(Register::CONFIG,
                             static_cast<uint8_t>(ConfigBits::DLPF_CFG_LSB), 3,
                             static_cast<uint8_t>(gyro_dlpf)));
  }

  return ESP_OK;
}

esp_err_t MPU9250::set_sample_rate(uint16_t rate_hz) {
  // TODO: assume 1kHz internal sample rate
  constexpr uint16_t INTERNAL_SAMPLE_RATE_HZ = 1000;
  uint8_t divider = INTERNAL_SAMPLE_RATE_HZ / rate_hz - 1;
  RETURN_ON_ERR(write_reg(Register::SMPLRT_DIV, divider));
  return ESP_OK;
}

esp_err_t MPU9250::read_reg(Register addr, uint8_t& data) {
  spi_transaction_t transaction = {.flags = 0,
                                   .cmd = 0,
                                   .addr = static_cast<uint8_t>(addr) | 0x80U,
                                   .length = 8,
                                   .rxlength = 8,
                                   .user = nullptr,
                                   .tx_buffer = nullptr,
                                   .rx_buffer = &data};

  RETURN_ON_ERR(spi_device_polling_transmit(spi_handle_, &transaction));
  return ESP_OK;
}

esp_err_t MPU9250::read_bytes(Register addr, size_t len, uint8_t* buffer) {
  spi_transaction_t transaction = {.flags = 0,
                                   .cmd = 0,
                                   .addr = static_cast<uint8_t>(addr) | 0x80U,
                                   .length = len * 8,
                                   .rxlength = len * 8,
                                   .user = nullptr,
                                   .tx_buffer = nullptr,
                                   .rx_buffer = buffer};

  RETURN_ON_ERR(spi_device_polling_transmit(spi_handle_, &transaction));
  return ESP_OK;
}

esp_err_t MPU9250::write_reg(Register addr, uint8_t data) {
  spi_transaction_t transaction = {.flags = 0,
                                   .cmd = 0,
                                   .addr = static_cast<uint8_t>(addr),
                                   .length = 8,
                                   .rxlength = 8,
                                   .user = nullptr,
                                   .tx_buffer = &data,
                                   .rx_buffer = nullptr};

  RETURN_ON_ERR(spi_device_polling_transmit(spi_handle_, &transaction));
  return ESP_OK;
}

esp_err_t MPU9250::write_bit(Register addr, uint8_t bit, bool set) {
  uint8_t reg_val;

  RETURN_ON_ERR(read_reg(addr, reg_val));
  reg_val &= ~(0x01U << bit);
  reg_val |= (static_cast<uint8_t>(set) << bit);
  RETURN_ON_ERR(write_reg(addr, reg_val));

  return ESP_OK;
}

esp_err_t MPU9250::write_bits(Register addr, uint8_t lsb, size_t len,
                              uint8_t data) {
  uint8_t reg_val;
  uint8_t mask = 0;

  RETURN_ON_ERR(read_reg(addr, reg_val));
  for (uint8_t i = lsb; i < lsb + len; i++) {
    mask |= (0x01U << i);
  }
  reg_val &= ~mask;
  reg_val |= (data << lsb) & mask;
  RETURN_ON_ERR(write_reg(addr, reg_val));

  return ESP_OK;
}
