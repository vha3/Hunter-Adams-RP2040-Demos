# cmake version
cmake_minimum_required(VERSION 3.13)

# include the sdk.cmake file
include(pico_sdk_import.cmake)

# give the project a name (anything you want)
project(Audio_Timer_Interrupt_DDS C CXX ASM)

# initialize the sdk
pico_sdk_init()

add_executable(Audio_Multicore_DDS_Demo multicore_dds.c)

target_sources(Audio_Multicore_DDS_Demo PRIVATE multicore_dds.c)

# Add pico_multicore which is required for multicore functionality
target_link_libraries(Audio_Multicore_DDS_Demo pico_stdlib pico_multicore pico_bootsel_via_double_reset hardware_sync hardware_spi hardware_timer hardware_irq)

# create map/bin/hex file etc.
pico_add_extra_outputs(Audio_Multicore_DDS_Demo)