#include <stdio.h>
#include "battery_monitor_task.h"
#include "data_buffer.h"
#include "freertos/FreeRTOS.h"
#include "imu_read_task.h"
#include "sdkconfig.h"
#include "shared.h"
#include "task.h"

// Constants
static constexpr size_t DEFAULT_TASK_STACK_SIZE = 8192;

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

// Battery Monitor Task
static StackType_t battery_monitor_task_stack[DEFAULT_TASK_STACK_SIZE];
static Task::Config battery_monitor_task_config = {
    .name = "Battery Monitor Task",
    .stack_depth = DEFAULT_TASK_STACK_SIZE,
    .priority = 6,
    .stack_buffer = battery_monitor_task_stack,
    .core_id = 0,
};
static BatteryMonitorTask battery_monitor_task(battery_monitor_task_config);

extern "C" {
void app_main(void) {
  imu_read_task.create();
  battery_monitor_task.create();
}
}
