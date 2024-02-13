cmake_minimum_required(VERSION 3.16)
idf_component_register(SRCS "rgb_blinky.cpp"
                    INCLUDE_DIRS ""
                    REQUIRES gpio rgb_led)