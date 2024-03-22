idf_component_register(SRCS "app_main.cpp"
                    INCLUDE_DIRS ""
                    REQUIRES common battery_monitor_task imu_read_task state_machine_task networking_task config_manager_task telemetry_task)
