idf_component_register(SRCS "battery_monitor_task.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES gpio common esp_adc ema_filter)
