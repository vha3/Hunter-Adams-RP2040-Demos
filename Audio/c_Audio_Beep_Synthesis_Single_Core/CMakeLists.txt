# cmake version
cmake_minimum_required(VERSION 3.13)

# include the sdk.cmake file
include(pico_sdk_import.cmake)

# give the project a name (anything you want)
project(Audio_Beep_Synth_Single_Core C CXX ASM)

# initialize the sdk
pico_sdk_init()

add_executable(Audio_Beep_Synth_Single_Core)

target_sources(Audio_Beep_Synth_Single_Core PRIVATE beep_beep.c)

# Add pico_multicore which is required for multicore functionality
target_link_libraries(Audio_Beep_Synth_Single_Core pico_stdlib pico_bootsel_via_double_reset hardware_spi hardware_sync)

# create map/bin/hex file etc.
pico_add_extra_outputs(Audio_Beep_Synth_Single_Core)