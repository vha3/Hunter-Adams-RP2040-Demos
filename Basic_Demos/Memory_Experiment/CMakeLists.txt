# cmake version
cmake_minimum_required(VERSION 3.13)

# include the sdk.cmake file
include(pico_sdk_import.cmake)

# give the project a name (anything you want)
project(Memory_Experiment C CXX ASM)

# initialize the sdk
pico_sdk_init()

# add executable
add_executable(Memory_Experiment)

# add sources
target_sources(Memory_Experiment PRIVATE memory_experimentation.c)

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(Memory_Experiment pico_stdlib pico_bootsel_via_double_reset)

# pico_set_binary_type(Memory_Experiment no_flash)

# create map/bin/hex file etc.
pico_add_extra_outputs(Memory_Experiment)