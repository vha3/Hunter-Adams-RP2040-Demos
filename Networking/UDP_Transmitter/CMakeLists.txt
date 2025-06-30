# cmake version
cmake_minimum_required(VERSION 3.13)

# include the sdk.cmake file
include(pico_sdk_import.cmake)

# give the project a name (anything you want)
project(UDP_Transmitter_PIO C CXX ASM)

# initialize the sdk
pico_sdk_init()

add_executable(UDP_Transmitter_PIO)

pico_generate_pio_header(UDP_Transmitter_PIO ${CMAKE_CURRENT_LIST_DIR}/udp_transmit.pio)

target_sources(UDP_Transmitter_PIO PRIVATE udp_tx_demo.c)

target_link_libraries(UDP_Transmitter_PIO PRIVATE pico_stdlib pico_bootsel_via_double_reset pico_unique_id hardware_pio hardware_dma hardware_pwm)

pico_add_extra_outputs(UDP_Transmitter_PIO)
