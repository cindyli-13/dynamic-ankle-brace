#include "mpu9250.h"

void MPU9250::init(const Config& config) {
    esp_err_t ret;
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
        .post_cb = nullptr
    };

    ret = spi_bus_add_device(spi_host_, &devcfg, &spi_handle_);
    ESP_ERROR_CHECK(ret);

    reset();
    set_sleep(false);
    disable_i2c(false);
    set_clock_source(ClockSource::PLL);

    // Verify device whoami
    ret = (whoami() == DEFAULT_WHOAMI ? ESP_OK : ESP_ERR_INVALID_RESPONSE);
    ESP_ERROR_CHECK(ret);

    set_fsr(config.accel_fsr, config.gyro_fsr);
    set_dlpf(config.accel_dlpf, config.gyro_dlpf);
    set_sample_rate(config.sample_rate_hz);
}

void MPU9250::read(RawImuData& imu_data) {
    uint8_t buffer[14] = {0};
    read_bytes(Register::ACCEL_XOUT_H, 14, buffer);
    imu_data.accel.x = (static_cast<uint16_t>(buffer[0]) << 8) | static_cast<uint16_t>(buffer[1]);
    imu_data.accel.y = (static_cast<uint16_t>(buffer[2]) << 8) | static_cast<uint16_t>(buffer[3]);
    imu_data.accel.z = (static_cast<uint16_t>(buffer[4]) << 8) | static_cast<uint16_t>(buffer[5]);
    imu_data.temperature = (static_cast<uint16_t>(buffer[6]) << 8) | static_cast<uint16_t>(buffer[7]);
    imu_data.gyro.x = (static_cast<uint16_t>(buffer[8]) << 8) | static_cast<uint16_t>(buffer[9]);
    imu_data.gyro.y = (static_cast<uint16_t>(buffer[10]) << 8) | static_cast<uint16_t>(buffer[11]);
    imu_data.gyro.z = (static_cast<uint16_t>(buffer[12]) << 8) | static_cast<uint16_t>(buffer[13]);
}

void MPU9250::reset() {
    write_bit(Register::PWR_MGMT_1, static_cast<uint8_t>(PwrMgmt1Bits::H_RESET), 1);
    write_bit(Register::USER_CTRL, static_cast<uint8_t>(UserCtrlBits::SIG_COND_RST), 1);
}

void MPU9250::set_sleep(bool sleep) {
    write_bit(Register::PWR_MGMT_1, static_cast<uint8_t>(PwrMgmt1Bits::SLEEP), sleep);
}

void MPU9250::disable_i2c(bool disable) {
    write_bit(Register::USER_CTRL, static_cast<uint8_t>(UserCtrlBits::I2C_IF_DIS), disable);
}

void MPU9250::set_clock_source(ClockSource clock_source) {
    write_bits(Register::PWR_MGMT_1, static_cast<uint8_t>(PwrMgmt1Bits::CLK_SEL_LSB), 3, static_cast<uint8_t>(clock_source));
}

uint8_t MPU9250::whoami() {
    return read_reg(Register::WHOAMI);
}

void MPU9250::set_fsr(AccelFSR accel_fsr, GyroFSR gyro_fsr) {
    write_bits(Register::ACCEL_CONFIG, static_cast<uint8_t>(AccelConfigBits::FS_SEL_LSB), 2, static_cast<uint8_t>(accel_fsr));
    write_bits(Register::GYRO_CONFIG, static_cast<uint8_t>(GyroConfigBits::FS_SEL_LSB), 2, static_cast<uint8_t>(gyro_fsr));
}

void MPU9250::set_dlpf(AccelDLPF accel_dlpf, GyroDLPF gyro_dlpf) {
    if (accel_dlpf == AccelDLPF::NONE) {
        write_bit(Register::ACCEL_CONFIG_2, static_cast<uint8_t>(AccelConfig2Bits::F_CHOICE_B), 1);
    }
    else {
        write_bit(Register::ACCEL_CONFIG_2, static_cast<uint8_t>(AccelConfig2Bits::F_CHOICE_B), 0);
        write_bits(Register::ACCEL_CONFIG_2, static_cast<uint8_t>(AccelConfig2Bits::A_DLPFCFG_LSB), 3, static_cast<uint8_t>(accel_dlpf));
    }

    if (gyro_dlpf == GyroDLPF::NONE) {
        write_bits(Register::GYRO_CONFIG, static_cast<uint8_t>(GyroConfigBits::F_CHOICE_B_LSB), 2, 0b11);
    }
    else {
        write_bits(Register::GYRO_CONFIG, static_cast<uint8_t>(GyroConfigBits::F_CHOICE_B_LSB), 2, 0b00);
        write_bits(Register::CONFIG, static_cast<uint8_t>(ConfigBits::DLPF_CFG_LSB), 3, static_cast<uint8_t>(gyro_dlpf));
    }
}

void MPU9250::set_sample_rate(uint16_t rate_hz) {
    // TODO: assume 1kHz internal sample rate
    constexpr uint16_t INTERNAL_SAMPLE_RATE_HZ = 1000;
    uint8_t divider = INTERNAL_SAMPLE_RATE_HZ / rate_hz - 1;
    write_reg(Register::SMPLRT_DIV, divider);
}

uint8_t MPU9250::read_reg(Register addr) {
    esp_err_t ret;
    uint8_t rx_buffer;

    spi_transaction_t transaction = {
        .flags = 0,
        .cmd = 0,
        .addr = static_cast<uint8_t>(addr) | 0x80U,
        .length = 8,
        .rxlength = 8,
        .user = nullptr,
        .tx_buffer = nullptr,
        .rx_buffer = &rx_buffer
    };

    ret = spi_device_polling_transmit(spi_handle_, &transaction);
    ESP_ERROR_CHECK(ret);

    return rx_buffer;
}

void MPU9250::read_bytes(Register addr, size_t len, uint8_t* buffer) {
    esp_err_t ret;
    spi_transaction_t transaction = {
        .flags = 0,
        .cmd = 0,
        .addr = static_cast<uint8_t>(addr) | 0x80U,
        .length = len * 8,
        .rxlength = len * 8,
        .user = nullptr,
        .tx_buffer = nullptr,
        .rx_buffer = buffer
    };

    ret = spi_device_polling_transmit(spi_handle_, &transaction);
    ESP_ERROR_CHECK(ret);
}

void MPU9250::write_reg(Register addr, uint8_t data) {
    esp_err_t ret;

    spi_transaction_t transaction = {
        .flags = 0,
        .cmd = 0,
        .addr = static_cast<uint8_t>(addr),
        .length = 8,
        .rxlength = 8,
        .user = nullptr,
        .tx_buffer = &data,
        .rx_buffer = nullptr
    };

    ret = spi_device_polling_transmit(spi_handle_, &transaction);
    ESP_ERROR_CHECK(ret);
}

void MPU9250::write_bit(Register addr, uint8_t bit, bool set) {
    uint8_t reg_val = read_reg(addr);
    reg_val &= ~(0x01U << bit);
    reg_val |= (static_cast<uint8_t>(set) << bit);
    write_reg(addr, reg_val);
}

void MPU9250::write_bits(Register addr, uint8_t lsb, size_t len, uint8_t data) {
    uint8_t reg_val = read_reg(addr);
    uint8_t mask = 0;
    for (uint8_t i = lsb; i < lsb+len; i++) {
        mask |= (0x01U << i);
    }
    reg_val &= ~mask;
    reg_val |= (data << lsb) & mask;
    write_reg(addr, reg_val);
}
