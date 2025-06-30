# cmake version
cmake_minimum_required(VERSION 3.13)

# include the sdk.cmake file
include(pico_sdk_import.cmake)

# give the project a name (anything you want)
project(PWM_Radio_Beacon C CXX ASM)

# initialize the sdk
pico_sdk_init()

add_executable(PWM_Radio_Beacon)

# must match with executable name and source file names
target_sources(PWM_Radio_Beacon PRIVATE am-beacon.c)

# Add pico_multicore which is required for multicore functionality
target_link_libraries(PWM_Radio_Beacon pico_stdlib pico_multicore pico_bootsel_via_double_reset hardware_pwm)

# create map/bin/hex file etc.
pico_add_extra_outputs(PWM_Radio_Beacon)