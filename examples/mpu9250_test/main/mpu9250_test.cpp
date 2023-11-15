#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mpu9250.h"

extern "C" {

void app_main(void) {
    printf("MPU 9250 Test\n");

    MPU9250 imu;
    imu.init();

    while (true) {
        uint8_t whoami = imu.get_whoami();
        printf("whoami: %d\n", whoami);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

}
