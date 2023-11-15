#include "mpu9250.h"

#define SPI_HOST     SPI2_HOST

#define PIN_NUM_MISO 13
#define PIN_NUM_MOSI 11
#define PIN_NUM_CLK  12
#define PIN_NUM_CS   10

#define READ_BIT     0x80U

#define WHOAMI_REG   0x75U

void MPU9250::init() {
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

    spi_device_interface_config_t devcfg = {
        .command_bits = 0,
        .address_bits = 8,
        .dummy_bits = 0,
        .mode = 3,
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .duty_cycle_pos = 0,
        .cs_ena_pretrans = 0,
        .cs_ena_posttrans = 0,
        .clock_speed_hz = 1000,
        .input_delay_ns = 0,
        .spics_io_num = PIN_NUM_CS,
        .flags = 0,
        .queue_size = 8,
        .pre_cb = nullptr,
        .post_cb = nullptr
    };

    ret = spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    ret = spi_bus_add_device(SPI_HOST, &devcfg, &spi_handle_);
    ESP_ERROR_CHECK(ret);
}

uint8_t MPU9250::get_whoami() {
    esp_err_t ret;
    uint8_t rx;

    spi_transaction_t transaction = {
        .flags = 0,
        .cmd = 0,
        .addr = WHOAMI_REG | READ_BIT,
        .length = 8,
        .rxlength = 8,
        .user = nullptr,
        .tx_buffer = nullptr,
        .rx_buffer = &rx
    };

    ret = spi_device_polling_transmit(spi_handle_, &transaction);
    ESP_ERROR_CHECK(ret);

    return rx;
}
