#include <stdio.h>
#include "data_buffer.h"
#include "freertos/FreeRTOS.h"
#include "imu_read_task.h"
#include "sdkconfig.h"
#include "shared.h"
#include "task.h"

// Constants
static constexpr size_t DEFAULT_TASK_STACK_SIZE = 1024;

// Shared buffers
static shared::IMUDataBuffer imu_data_buffer;

// IMU Read Task
static IMUReadTask::Param imu_read_task_param = {.imu_data_buffer =
                                                     &imu_data_buffer};
static StackType_t imu_read_task_stack[DEFAULT_TASK_STACK_SIZE];
static Task::Config imu_read_task_config = {
    .name = "IMU Read Task",
    .stack_depth = DEFAULT_TASK_STACK_SIZE,
    .priority = 10,
    .stack_buffer = imu_read_task_stack,
    .core_id = 0,
};
static IMUReadTask imu_read_task(imu_read_task_config, &imu_read_task_param);

extern "C" {
void app_main(void) {
  imu_read_task.create();
}
}
