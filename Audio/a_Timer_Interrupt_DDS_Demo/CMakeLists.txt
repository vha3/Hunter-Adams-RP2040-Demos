# cmake version
cmake_minimum_required(VERSION 3.13)

# include the sdk.cmake file
include(pico_sdk_import.cmake)

# give the project a name (anything you want)
project(Audio_Timer_Interrupt_DDS C CXX ASM)

# initialize the sdk
pico_sdk_init()

add_executable(Audio_Timer_Interrupt_DDS)

target_sources(Audio_Timer_Interrupt_DDS PRIVATE dactest.c)

target_link_libraries(Audio_Timer_Interrupt_DDS pico_stdlib pico_bootsel_via_double_reset hardware_spi hardware_timer hardware_irq)

pico_add_extra_outputs(Audio_Timer_Interrupt_DDS)