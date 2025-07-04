# cmake version
cmake_minimum_required(VERSION 3.13)

# include the sdk.cmake file
include(pico_sdk_import.cmake)

# give the project a name (anything you want)
project(Resistive_Touchscreen C CXX ASM)

# initialize the sdk
pico_sdk_init()

add_executable(Resistive_Touchscreen)

# must match with pio filename and executable name from above
pico_generate_pio_header(Resistive_Touchscreen ${CMAKE_CURRENT_LIST_DIR}/hsync.pio)
pico_generate_pio_header(Resistive_Touchscreen ${CMAKE_CURRENT_LIST_DIR}/vsync.pio)
pico_generate_pio_header(Resistive_Touchscreen ${CMAKE_CURRENT_LIST_DIR}/rgb.pio)

# must match with executable name and source file names
target_sources(Resistive_Touchscreen PRIVATE trackpad.c vga16_graphics.c)

# must match with executable name
target_link_libraries(Resistive_Touchscreen PRIVATE pico_stdlib pico_bootsel_via_double_reset hardware_pio hardware_dma hardware_adc hardware_irq hardware_adc)

# must match with executable name
pico_add_extra_outputs(Resistive_Touchscreen)
