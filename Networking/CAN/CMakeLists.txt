# cmake version
cmake_minimum_required(VERSION 3.13)

# include the sdk.cmake file
include(pico_sdk_import.cmake)

# give the project a name (anything you want)
project(CAN_Bus C CXX ASM)

# initialize the sdk
pico_sdk_init()

add_executable(CAN_Bus)

pico_generate_pio_header(CAN_Bus ${CMAKE_CURRENT_LIST_DIR}/can.pio)

target_sources(CAN_Bus PRIVATE can_demo.c)

target_link_libraries(CAN_Bus PRIVATE pico_stdlib pico_multicore pico_bootsel_via_double_reset hardware_pio hardware_dma hardware_watchdog)

pico_add_extra_outputs(CAN_Bus)
