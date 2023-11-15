// MPU 9250 IMU driver

#include <inttypes.h>

#include "driver/spi_master.h"

class MPU9250 {

public:
    void init();
    uint8_t get_whoami();

private:
    spi_device_handle_t spi_handle_;
};
