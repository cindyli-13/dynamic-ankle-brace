// MPU 9250 IMU driver (SPI interface)
// Datasheet: https://invensense.tdk.com/wp-content/uploads/2015/02/RM-MPU-9250A-00-v1.6.pdf
// Register map: https://invensense.tdk.com/wp-content/uploads/2015/02/RM-MPU-9250A-00-v1.6.pdf

#include <inttypes.h>

#include "driver/spi_master.h"

class MPU9250 {

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
    int16_t temperature;
  };

  enum class GyroFSR : uint8_t {
    _250DPS = 0,
    _500DPS = 1,
    _1000DPS = 2,
    _2000DPS = 3
  };

  enum class AccelFSR : uint8_t { _2G = 0, _4G = 1, _8G = 2, _16G = 3 };

  enum class GyroDLPF : uint8_t {
    _250HZ = 0,
    _184HZ = 1,
    _92HZ = 2,
    _41HZ = 3,
    _20HZ = 4,
    _10HZ = 5,
    _5HZ = 6,
    _3600HZ = 7,
    NONE = 0xFFU
  };

  enum class AccelDLPF : uint8_t {
    _218_1HZ = 1,
    _99HZ = 2,
    _44_8HZ = 3,
    _21_2HZ = 4,
    _10_2HZ = 5,
    _5_05HZ = 6,
    _420HZ = 7,
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
    return (static_cast<float>(2 << static_cast<uint8_t>(fsr)) / INT16_MAX) *
           accel_raw;
  }

  static constexpr float GYRO_RAW_TO_DPS(int16_t gyro_raw, GyroFSR fsr) {
    return (static_cast<float>(250 << static_cast<uint8_t>(fsr)) / INT16_MAX) *
           gyro_raw;
  }

  static constexpr float TEMPERAURE_RAW_TO_DEG_C(int16_t temperature_raw) {
    // TODO: temperature offset and sensitivity not documented in datasheet, values taken from
    // https://github.com/kriswiner/MPU9250/blob/72038b040bef3cf072612cd8a8ee8f26e3c87158/MPU9250BasicAHRS.ino#L457
    return static_cast<float>(temperature_raw) / 333.87 + 21.0;
  }

  MPU9250(spi_host_device_t spi_host) : spi_host_(spi_host) {}

  void init(const Config& config);
  void read(RawImuData& imu_data);

 private:
  enum class Register : uint8_t {
    SMPLRT_DIV = 0x19U,
    CONFIG = 0x1AU,
    GYRO_CONFIG = 0x1BU,
    ACCEL_CONFIG = 0x1CU,
    ACCEL_CONFIG_2 = 0x1DU,
    ACCEL_XOUT_H = 0x3BU,
    USER_CTRL = 0x6AU,
    PWR_MGMT_1 = 0x6BU,
    WHOAMI = 0x75U
  };

  enum class ConfigBits : uint8_t { DLPF_CFG_LSB = 0 };

  enum class GyroConfigBits : uint8_t { FS_SEL_LSB = 3, F_CHOICE_B_LSB = 0 };

  enum class AccelConfigBits : uint8_t { FS_SEL_LSB = 3 };

  enum class AccelConfig2Bits : uint8_t { F_CHOICE_B = 3, A_DLPFCFG_LSB = 0 };

  enum class UserCtrlBits : uint8_t { SIG_COND_RST = 0, I2C_IF_DIS = 4 };

  enum class PwrMgmt1Bits : uint8_t { CLK_SEL_LSB = 0, SLEEP = 6, H_RESET = 7 };

  enum class ClockSource : uint8_t {
    InternalOscillator = 0,
    PLL = 3,
    KeepReset = 7
  };

  static constexpr uint8_t DEFAULT_WHOAMI = 0x71U;

  spi_host_device_t spi_host_;
  spi_device_handle_t spi_handle_;

  void reset();
  void set_sleep(bool sleep);
  void disable_i2c(bool disable);
  void set_clock_source(ClockSource clock_source);
  uint8_t whoami();
  void set_fsr(AccelFSR accel_fsr, GyroFSR gyro_fsr);
  void set_dlpf(AccelDLPF accel_dlpf, GyroDLPF gyro_dlpf);
  void set_sample_rate(uint16_t rate_hz);

  uint8_t read_reg(Register addr);
  void read_bytes(Register addr, size_t len, uint8_t* buffer);
  void write_reg(Register addr, uint8_t data);
  void write_bit(Register addr, uint8_t bit, bool set);
  void write_bits(Register addr, uint8_t lsb, size_t len, uint8_t data);
};
