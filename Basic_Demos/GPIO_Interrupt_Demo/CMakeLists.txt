# cmake version
cmake_minimum_required(VERSION 3.13)

# include the sdk.cmake file
include(pico_sdk_import.cmake)

# give the project a name (anything you want)
project(GPIO_Interrupt_Demo C CXX ASM)

# initialize the sdk
pico_sdk_init()

# add executable
add_executable(GPIO_Interrupt_Demo)

# add sources
target_sources(GPIO_Interrupt_Demo PRIVATE irq_timing.c)

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(GPIO_Interrupt_Demo pico_stdlib pico_bootsel_via_double_reset)

# create map/bin/hex file etc.
pico_add_extra_outputs(GPIO_Interrupt_Demo)