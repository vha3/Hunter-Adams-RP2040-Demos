# cmake version
cmake_minimum_required(VERSION 3.13)

# include the sdk.cmake file
include(pico_sdk_import.cmake)

# give the project a name (anything you want)
project(PWM_Demo C CXX ASM)

# initialize the sdk
pico_sdk_init()

add_executable(PWM_Demo)

# must match with executable name and source file names
target_sources(PWM_Demo PRIVATE pwm-demo.c)

# Add pico_multicore which is required for multicore functionality
target_link_libraries(PWM_Demo pico_stdlib pico_multicore pico_bootsel_via_double_reset hardware_pwm hardware_irq)

# create map/bin/hex file etc.
pico_add_extra_outputs(PWM_Demo)