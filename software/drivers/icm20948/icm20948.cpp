#include "icm20948.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// TODO: figure out best way to do error handling
// ESP-IDF has existing error check macros under esp_check.h but none exactly like this one:
#define RETURN_ON_ERR(x) \
  do {                   \
    esp_err_t err = (x); \
    if (err != ESP_OK) { \
      return err;        \
    }                    \
  } while (0)

esp_err_t ICM20948::init(const Config& config) {
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

  // Verify device whoami
  uint8_t data;
  RETURN_ON_ERR(whoami(data));
  if (data != DEFAULT_WHOAMI) {
    return ESP_ERR_INVALID_RESPONSE;
  }

  RETURN_ON_ERR(reset());
  vTaskDelay(1);
  RETURN_ON_ERR(set_sleep(false));
  RETURN_ON_ERR(disable_i2c(true));
  RETURN_ON_ERR(set_low_power(false));
  RETURN_ON_ERR(init_interrupt(false));

  RETURN_ON_ERR(set_accel_config(config.accel_fsr, config.accel_dlpf));
  RETURN_ON_ERR(set_gyro_config(config.gyro_fsr, config.gyro_dlpf));
  RETURN_ON_ERR(set_sample_rate(config.sample_rate_hz));

  return ESP_OK;
}

esp_err_t ICM20948::read(RawImuData& imu_data) {
  uint8_t buffer[12] = {0};

  RETURN_ON_ERR(set_bank(0));
  RETURN_ON_ERR(
      read_bytes(static_cast<uint8_t>(RegAddrBank0::ACCEL_XOUT_H), 12, buffer));

  imu_data.accel.x = (static_cast<uint16_t>(buffer[0]) << 8) |
                     static_cast<uint16_t>(buffer[1]);
  imu_data.accel.y = (static_cast<uint16_t>(buffer[2]) << 8) |
                     static_cast<uint16_t>(buffer[3]);
  imu_data.accel.z = (static_cast<uint16_t>(buffer[4]) << 8) |
                     static_cast<uint16_t>(buffer[5]);
  imu_data.gyro.x = (static_cast<uint16_t>(buffer[6]) << 8) |
                    static_cast<uint16_t>(buffer[7]);
  imu_data.gyro.y = (static_cast<uint16_t>(buffer[8]) << 8) |
                    static_cast<uint16_t>(buffer[9]);
  imu_data.gyro.z = (static_cast<uint16_t>(buffer[10]) << 8) |
                    static_cast<uint16_t>(buffer[11]);

  return ESP_OK;
}

esp_err_t ICM20948::reset() {
  uint8_t reg_val;

  RETURN_ON_ERR(set_bank(0));
  RETURN_ON_ERR(
      read_reg(static_cast<uint8_t>(RegAddrBank0::PWR_MGMT_1), reg_val));

  reg_val |= static_cast<uint8_t>(PwrMgmt1Bits::DEVICE_RESET);

  RETURN_ON_ERR(
      write_reg(static_cast<uint8_t>(RegAddrBank0::PWR_MGMT_1), reg_val));
  return ESP_OK;
}

esp_err_t ICM20948::set_sleep(bool sleep) {
  uint8_t reg_val;

  RETURN_ON_ERR(set_bank(0));
  RETURN_ON_ERR(
      read_reg(static_cast<uint8_t>(RegAddrBank0::PWR_MGMT_1), reg_val));

  if (sleep) {
    reg_val |= static_cast<uint8_t>(PwrMgmt1Bits::SLEEP);
  } else {
    reg_val &= ~static_cast<uint8_t>(PwrMgmt1Bits::SLEEP);
  }

  RETURN_ON_ERR(
      write_reg(static_cast<uint8_t>(RegAddrBank0::PWR_MGMT_1), reg_val));
  return ESP_OK;
}

esp_err_t ICM20948::disable_i2c(bool disable) {
  uint8_t reg_val;

  RETURN_ON_ERR(set_bank(0));
  RETURN_ON_ERR(
      read_reg(static_cast<uint8_t>(RegAddrBank0::USER_CTRL), reg_val));

  if (disable) {
    reg_val |= static_cast<uint8_t>(UserCtrlBits::I2C_IF_DIS);
  } else {
    reg_val &= ~static_cast<uint8_t>(UserCtrlBits::I2C_IF_DIS);
  }

  RETURN_ON_ERR(
      write_reg(static_cast<uint8_t>(RegAddrBank0::USER_CTRL), reg_val));
  return ESP_OK;
}

esp_err_t ICM20948::whoami(uint8_t& val) {
  RETURN_ON_ERR(set_bank(0));
  RETURN_ON_ERR(read_reg(static_cast<uint8_t>(RegAddrBank0::WHO_AM_I), val));
  return ESP_OK;
}

esp_err_t ICM20948::set_low_power(bool enable) {
  uint8_t reg_val;

  RETURN_ON_ERR(set_bank(0));
  RETURN_ON_ERR(
      read_reg(static_cast<uint8_t>(RegAddrBank0::PWR_MGMT_1), reg_val));

  if (enable) {
    reg_val |= static_cast<uint8_t>(PwrMgmt1Bits::LP_ENABLE);
  } else {
    reg_val &= ~static_cast<uint8_t>(PwrMgmt1Bits::LP_ENABLE);
  }

  RETURN_ON_ERR(
      write_reg(static_cast<uint8_t>(RegAddrBank0::PWR_MGMT_1), reg_val));
  return ESP_OK;
}

esp_err_t ICM20948::init_interrupt(bool active_low) {
  uint8_t reg_val;

  RETURN_ON_ERR(set_bank(0));
  RETURN_ON_ERR(
      read_reg(static_cast<uint8_t>(RegAddrBank0::INT_PIN_CONFIG), reg_val));

  // Configure interrupt active low/high
  if (active_low) {
    reg_val |= static_cast<uint8_t>(IntPinConfigBits::INT1_ACTL);
  } else {
    reg_val &= ~static_cast<uint8_t>(IntPinConfigBits::INT1_ACTL);
  }
  reg_val &= ~static_cast<uint8_t>(
      IntPinConfigBits::INT1_OPEN);  // Configure interrupt as push-pull
  reg_val &= ~static_cast<uint8_t>(
      IntPinConfigBits::
          INT1_LATCH__EN);  // Configure interrupt pulse width as 50us
  reg_val |= static_cast<uint8_t>(
      IntPinConfigBits::
          INT_ANYRD_2CLEAR);  // Clear interrupt when any read is performed

  // Set interrupt config
  RETURN_ON_ERR(
      write_reg(static_cast<uint8_t>(RegAddrBank0::INT_PIN_CONFIG), reg_val));

  // Enable raw data ready interrupt
  RETURN_ON_ERR(
      read_reg(static_cast<uint8_t>(RegAddrBank0::INT_ENABLE_1), reg_val));
  reg_val |= static_cast<uint8_t>(IntEnable1Bits::RawData0Ready);
  RETURN_ON_ERR(
      write_reg(static_cast<uint8_t>(RegAddrBank0::INT_ENABLE_1), reg_val));

  return ESP_OK;
}

esp_err_t ICM20948::set_accel_config(AccelFSR fsr, AccelDLPF dlpf) {
  uint8_t config;

  RETURN_ON_ERR(set_bank(2));
  RETURN_ON_ERR(
      read_reg(static_cast<uint8_t>(RegAddrBank2::ACCEL_CONFIG), config));

  // Configure DLPF
  if (dlpf == AccelDLPF::NONE) {
    config &= ~static_cast<uint8_t>(AccelConfigBits::ACCEL_F_CHOICE);
  } else {
    config |= static_cast<uint8_t>(AccelConfigBits::ACCEL_F_CHOICE);
    config &= ~static_cast<uint8_t>(AccelConfigBits::ACCEL_DLPFCFG);
    config |= (static_cast<uint8_t>(dlpf) << 3);
  }

  // Configure FS
  config &= ~static_cast<uint8_t>(AccelConfigBits::ACCEL_FS_SEL);
  config |= static_cast<uint8_t>(fsr) << 1;

  RETURN_ON_ERR(
      write_reg(static_cast<uint8_t>(RegAddrBank2::ACCEL_CONFIG), config));

  return ESP_OK;
}

esp_err_t ICM20948::set_gyro_config(GyroFSR fsr, GyroDLPF dlpf) {
  uint8_t config;

  RETURN_ON_ERR(set_bank(2));
  RETURN_ON_ERR(
      read_reg(static_cast<uint8_t>(RegAddrBank2::GYRO_CONFIG_1), config));

  // Configure DLPF
  if (dlpf == GyroDLPF::NONE) {
    config &= ~static_cast<uint8_t>(GyroConfig1Bits::GYRO_F_CHOICE);
  } else {
    config |= static_cast<uint8_t>(GyroConfig1Bits::GYRO_F_CHOICE);
    config &= ~static_cast<uint8_t>(GyroConfig1Bits::GYRO_DLPFCFG);
    config |= (static_cast<uint8_t>(dlpf) << 3);
  }

  // Configure FS
  config &= ~static_cast<uint8_t>(GyroConfig1Bits::GYRO_FS_SEL);
  config |= static_cast<uint8_t>(fsr) << 1;

  RETURN_ON_ERR(
      write_reg(static_cast<uint8_t>(RegAddrBank2::GYRO_CONFIG_1), config));

  return ESP_OK;
}

esp_err_t ICM20948::set_sample_rate(uint16_t rate_hz) {
  RETURN_ON_ERR(set_bank(2));

  // Set gyro sample rate
  uint8_t gyro_divider = 1100 / rate_hz - 1;
  RETURN_ON_ERR(write_reg(static_cast<uint8_t>(RegAddrBank2::GYRO_SMPLRT_DIV),
                          gyro_divider));

  // Set accel sample rate
  uint16_t accel_divider = 1125 / rate_hz - 1;
  uint8_t accel_divider_lsb = static_cast<uint8_t>(accel_divider & 0xFFU);
  uint8_t accel_divider_msb = static_cast<uint8_t>(accel_divider >> 8);
  RETURN_ON_ERR(
      write_reg(static_cast<uint8_t>(RegAddrBank2::ACCEL_SMPLRT_DIV_1),
                accel_divider_msb));
  RETURN_ON_ERR(
      write_reg(static_cast<uint8_t>(RegAddrBank2::ACCEL_SMPLRT_DIV_2),
                accel_divider_lsb));

  return ESP_OK;
}

esp_err_t ICM20948::set_bank(uint8_t bank) {
  if (bank > 3) {
    return ESP_ERR_INVALID_ARG;
  }
  if (curr_bank_ == bank) {
    return ESP_OK;
  }

  RETURN_ON_ERR(write_reg(static_cast<uint8_t>(RegAddrGeneral::BANK_SEL),
                          (bank << 4) & 0x30));
  curr_bank_ = bank;

  return ESP_OK;
}

esp_err_t ICM20948::read_reg(uint8_t addr, uint8_t& data) {
  spi_transaction_t transaction = {.flags = 0,
                                   .cmd = 0,
                                   .addr = addr | 0x80U,
                                   .length = 8,
                                   .rxlength = 8,
                                   .user = nullptr,
                                   .tx_buffer = nullptr,
                                   .rx_buffer = &data};

  RETURN_ON_ERR(spi_device_transmit(spi_handle_, &transaction));
  return ESP_OK;
}

esp_err_t ICM20948::read_bytes(uint8_t addr, size_t len, uint8_t* buffer) {
  spi_transaction_t transaction = {.flags = 0,
                                   .cmd = 0,
                                   .addr = addr | 0x80U,
                                   .length = len * 8,
                                   .rxlength = len * 8,
                                   .user = nullptr,
                                   .tx_buffer = nullptr,
                                   .rx_buffer = buffer};

  RETURN_ON_ERR(spi_device_transmit(spi_handle_, &transaction));
  return ESP_OK;
}

esp_err_t ICM20948::write_reg(uint8_t addr, uint8_t data) {
  spi_transaction_t transaction = {.flags = 0,
                                   .cmd = 0,
                                   .addr = addr,
                                   .length = 8,
                                   .rxlength = 8,
                                   .user = nullptr,
                                   .tx_buffer = &data,
                                   .rx_buffer = nullptr};

  RETURN_ON_ERR(spi_device_transmit(spi_handle_, &transaction));
  return ESP_OK;
}
