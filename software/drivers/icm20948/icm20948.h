// ICM 20948 IMU driver (SPI interface)
// Datasheet: https://invensense.tdk.com/wp-content/uploads/2021/10/DS-000189-ICM-20948-v1.5.pdf

#pragma once

#include <inttypes.h>

#include "driver/spi_master.h"

class ICM20948 {

 public:
  struct RawImuData {
    struct {
      int16_t x;
      int16_t y;
      int16_t z;
    } accel;
    struct {
      int16_t x;
      int16_t y;
      int16_t z;
    } gyro;
  };

  enum class GyroFSR : uint8_t {
    _250DPS = 0,
    _500DPS = 1,
    _1000DPS = 2,
    _2000DPS = 3
  };

  enum class AccelFSR : uint8_t { _2G = 0, _4G = 1, _8G = 2, _16G = 3 };

  enum class GyroDLPF : uint8_t {
    _196_6HZ = 0,
    _151_8HZ = 1,
    _119_5HZ = 2,
    _51_2HZ = 3,
    _23_HZ = 4,
    _11_6HZ = 5,
    _5_7HZ = 6,
    _361_4HZ = 7,
    NONE = 0xFFU
  };

  enum class AccelDLPF : uint8_t {
    _246HZ = 1,
    _111_4HZ = 2,
    _50_4HZ = 3,
    _23_9HZ = 4,
    _11_5HZ = 5,
    _5_7HZ = 6,
    _473HZ = 7,
    NONE = 0xFFU
  };

  struct Config {
    uint8_t cs_pin;
    int spi_clock_speed_hz;
    uint16_t sample_rate_hz;
    AccelFSR accel_fsr;
    GyroFSR gyro_fsr;
    AccelDLPF accel_dlpf;
    GyroDLPF gyro_dlpf;
  };

  static constexpr float ACCEL_RAW_TO_G(int16_t accel_raw, AccelFSR fsr) {
    return static_cast<float>(accel_raw) / 16384.0 *
           (1 << static_cast<uint8_t>(fsr));
  }

  static constexpr float GYRO_RAW_TO_DPS(int16_t gyro_raw, GyroFSR fsr) {
    return static_cast<float>(gyro_raw) / 131.0 *
           (1 << static_cast<uint8_t>(fsr));
  }

  ICM20948(spi_host_device_t spi_host)
      : spi_host_(spi_host), curr_bank_(0xFFU) {}

  esp_err_t init(const Config& config);
  esp_err_t read(RawImuData& imu_data);

 private:
  enum class RegAddrGeneral : uint8_t { BANK_SEL = 0x7FU };

  enum class RegAddrBank0 : uint8_t {
    WHO_AM_I = 0x00,
    USER_CTRL = 0x03,
    PWR_MGMT_1 = 0x06,
    INT_PIN_CONFIG = 0x0F,
    INT_ENABLE_1 = 0x11,
    I2C_MST_STATUS = 0x17,
    INT_STATUS = 0x19,
    INT_STATUS_1 = 0x1A,
    ACCEL_XOUT_H = 0x2D
  };

  enum class RegAddrBank2 : uint8_t {
    GYRO_SMPLRT_DIV = 0x00,
    GYRO_CONFIG_1 = 0x01,
    ACCEL_SMPLRT_DIV_1 = 0x10,
    ACCEL_SMPLRT_DIV_2 = 0x11,
    ACCEL_CONFIG = 0x14
  };

  enum class PwrMgmt1Bits : uint8_t {
    CLK_SEL = 0x07 << 0,
    TEMP_DIS = 0x01 << 3,
    LP_ENABLE = 0x01 << 5,
    SLEEP = 0x01 << 6,
    DEVICE_RESET = 0x01 << 7
  };

  enum class AccelConfigBits : uint8_t {
    ACCEL_F_CHOICE = 0x01 << 0,
    ACCEL_FS_SEL = 0x03 << 1,
    ACCEL_DLPFCFG = 0x07 << 3
  };

  enum class GyroConfig1Bits : uint8_t {
    GYRO_F_CHOICE = 0x01 << 0,
    GYRO_FS_SEL = 0x03 << 1,
    GYRO_DLPFCFG = 0x07 << 3
  };

  enum class IntPinConfigBits : uint8_t {
    BYPASS_EN = 0x01 << 1,
    FSYNC_INT_MODE_EN = 0x01 << 2,
    ACTL_FSYNC = 0x01 << 3,
    INT_ANYRD_2CLEAR = 0x01 << 4,
    INT1_LATCH__EN = 0x01 << 5,
    INT1_OPEN = 0x01 << 6,
    INT1_ACTL = 0x01 << 7
  };

  enum class IntEnable1Bits : uint8_t {
    RawData0Ready = 0x01 << 0,
  };

  enum class UserCtrlBits : uint8_t {
    I2C_MST_RST = 0x01 << 1,
    SRAM_RST = 0x01 << 2,
    DMP_RST = 0x01 << 3,
    I2C_IF_DIS = 0x01 << 4,
    I2C_MST_EN = 0x01 << 5,
    FIFO_EN = 0x01 << 6,
    DMP_EN = 0x01 << 7
  };

  static constexpr uint8_t DEFAULT_WHOAMI = 0xEA;

  spi_host_device_t spi_host_;
  spi_device_handle_t spi_handle_;
  uint8_t curr_bank_;

  esp_err_t reset();
  esp_err_t set_sleep(bool sleep);
  esp_err_t disable_i2c(bool disable);
  esp_err_t whoami(uint8_t& val);
  esp_err_t set_low_power(bool enable);
  esp_err_t set_accel_config(AccelFSR fsr, AccelDLPF dlpf);
  esp_err_t set_gyro_config(GyroFSR fsr, GyroDLPF dlpf);
  esp_err_t set_sample_rate(uint16_t rate_hz);

  esp_err_t set_bank(uint8_t bank);
  esp_err_t read_reg(uint8_t addr, uint8_t& data);
  esp_err_t read_bytes(uint8_t addr, size_t len, uint8_t* buffer);
  esp_err_t write_reg(uint8_t addr, uint8_t data);
};
