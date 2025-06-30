# cmake version
cmake_minimum_required(VERSION 3.13)

# include the sdk.cmake file
include(pico_sdk_import.cmake)

# give the project a name (anything you want)
project(Worm_Bootloader_application C CXX ASM)

# initialize the sdk
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

add_executable(Worm_Bootloader_application)

target_sources(Worm_Bootloader_application PRIVATE worm_blinky.c)

pico_set_linker_script(Worm_Bootloader_application ${CMAKE_CURRENT_SOURCE_DIR}/worm_application.ld)

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(Worm_Bootloader_application pico_stdlib pico_bootsel_via_double_reset hardware_dma hardware_uart hardware_flash hardware_irq hardware_watchdog)

# create map/bin/hex file etc.
pico_add_extra_outputs(Worm_Bootloader_application)