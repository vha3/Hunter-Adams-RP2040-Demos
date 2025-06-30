# cmake version
cmake_minimum_required(VERSION 3.13)

# include the sdk.cmake file
include(pico_sdk_import.cmake)

# give the project a name (anything you want)
project(Audio_DMA_Demo C CXX ASM)

# initialize the sdk
pico_sdk_init()

add_executable(Audio_DMA_Demo)

target_sources(Audio_DMA_Demo PRIVATE dma-demo.c)

target_link_libraries(Audio_DMA_Demo pico_stdlib pico_bootsel_via_double_reset hardware_dma hardware_spi)

# create map/bin/hex file etc.
pico_add_extra_outputs(Audio_DMA_Demo)

