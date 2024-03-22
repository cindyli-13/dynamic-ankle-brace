idf_component_register(SRCS "networking_task.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES common esp_wifi nvs_flash)