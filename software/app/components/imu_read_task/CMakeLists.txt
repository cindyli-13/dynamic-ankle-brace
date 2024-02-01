idf_component_register(SRCS "imu_read_task.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES icm20948 common)
