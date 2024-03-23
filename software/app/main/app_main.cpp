#include <stdio.h>
#include "battery_monitor_task.h"
#include "config_manager_task.h"
#include "data_buffer.h"
#include "freertos/FreeRTOS.h"
#include "imu_read_task.h"
#include "networking_task.h"
#include "sdkconfig.h"
#include "shared.h"
#include "state_machine_task.h"
#include "task.h"
#include "telemetry_task.h"

// Constants
static constexpr size_t DEFAULT_TASK_STACK_SIZE = 8192;

// Shared buffers
static shared::IMUDataBuffer imu_data_buffer;
static DataBuffer<float, 10> inversion_speed_buffer;
static DataBuffer<shared::State, 1> state_buffer;
static DataBuffer<bool, 1> calibration_requested_buffer;
static DataBuffer<shared::Config, 1> config_params_buffer;
static DataBuffer<float, 1> battery_voltage_buffer;

// IMU Read Task
static IMUReadTask::Param imu_read_task_param = {.imu_data_buffer =
                                                     &imu_data_buffer};
static StackType_t imu_read_task_stack[DEFAULT_TASK_STACK_SIZE];
static Task::Config imu_read_task_config = {
    .name = "IMU Read Task",
    .stack_depth = DEFAULT_TASK_STACK_SIZE,
    .priority = 10,
    .stack_buffer = imu_read_task_stack,
    .core_id = 1,
};
static IMUReadTask imu_read_task(imu_read_task_config, &imu_read_task_param);

// State Machine task
static StackType_t state_machine_task_stack[DEFAULT_TASK_STACK_SIZE];
static Task::Config state_machine_task_config = {
    .name = "State Machine Task",
    .stack_depth = DEFAULT_TASK_STACK_SIZE,
    .priority = 9,
    .stack_buffer = state_machine_task_stack,
    .core_id = 1,
};
static StateMachineTask::Param state_machine_task_param = {
    .imu_data_buffer = &imu_data_buffer,
    .inversion_speed_buffer = &inversion_speed_buffer,
    .state_buffer = &state_buffer,
    .calibration_requested_buffer = &calibration_requested_buffer,
    .config_params_buffer = &config_params_buffer,
};
static StateMachineTask state_machine_task(state_machine_task_config,
                                           &state_machine_task_param);

// Networking Task
static StackType_t networking_task_stack[DEFAULT_TASK_STACK_SIZE];
static Task::Config networking_task_config = {
    .name = "Networking Task",
    .stack_depth = DEFAULT_TASK_STACK_SIZE,
    .priority = 8,
    .stack_buffer = networking_task_stack,
    .core_id = 0,
};
static NetworkingTask::Param networking_task_param = {0};
static NetworkingTask networking_task(networking_task_config,
                                      &networking_task_param);

// Config Manager Task
static StackType_t config_manager_task_stack[DEFAULT_TASK_STACK_SIZE];
static Task::Config config_manager_task_config = {
    .name = "Networking Task",
    .stack_depth = DEFAULT_TASK_STACK_SIZE,
    .priority = 7,
    .stack_buffer = config_manager_task_stack,
    .core_id = 0,
};
static ConfigManagerTask::Param config_manager_task_param = {
    .calibration_requested_buffer = &calibration_requested_buffer,
    .config_params_buffer = &config_params_buffer,
};
static ConfigManagerTask config_manager_task(config_manager_task_config,
                                             &config_manager_task_param);

// Telemetry Task
static StackType_t telemetry_task_stack[DEFAULT_TASK_STACK_SIZE];
static Task::Config telemetry_task_config = {
    .name = "Networking Task",
    .stack_depth = DEFAULT_TASK_STACK_SIZE,
    .priority = 7,
    .stack_buffer = telemetry_task_stack,
    .core_id = 0,
};
static TelemetryTask::Param telemetry_task_param = {
    .inversion_speed_buffer = &inversion_speed_buffer,
    .state_buffer = &state_buffer,
    .battery_voltage_buffer = &battery_voltage_buffer,
};
static TelemetryTask telemetry_task(telemetry_task_config,
                                    &telemetry_task_param);

// Battery Monitor Task
static StackType_t battery_monitor_task_stack[DEFAULT_TASK_STACK_SIZE];
static Task::Config battery_monitor_task_config = {
    .name = "Battery Monitor Task",
    .stack_depth = DEFAULT_TASK_STACK_SIZE,
    .priority = 6,
    .stack_buffer = battery_monitor_task_stack,
    .core_id = 1,
};
static BatteryMonitorTask::Param battery_monitor_task_param = {
    .battery_voltage_buffer = &battery_voltage_buffer,
};
static BatteryMonitorTask battery_monitor_task(battery_monitor_task_config,
                                               &battery_monitor_task_param);

extern "C" {
void app_main(void) {
  imu_read_task.create();
  state_machine_task.create();
  networking_task.create();
  config_manager_task.create();
  telemetry_task.create();
  battery_monitor_task.create();
}
}
