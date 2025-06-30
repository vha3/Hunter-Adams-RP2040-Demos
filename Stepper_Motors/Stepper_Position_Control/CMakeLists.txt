# cmake version
cmake_minimum_required(VERSION 3.13)

# include the sdk.cmake file
include(pico_sdk_import.cmake)

# give the project a name (anything you want)
project(Stepper_Motors_position C CXX ASM)

# initialize the sdk
pico_sdk_init()

add_executable(Stepper_Motors_position)

pico_generate_pio_header(Stepper_Motors_position ${CMAKE_CURRENT_LIST_DIR}/stepper.pio)
pico_generate_pio_header(Stepper_Motors_position ${CMAKE_CURRENT_LIST_DIR}/counter.pio)
pico_generate_pio_header(Stepper_Motors_position ${CMAKE_CURRENT_LIST_DIR}/stepper1.pio)
pico_generate_pio_header(Stepper_Motors_position ${CMAKE_CURRENT_LIST_DIR}/counter1.pio)

target_sources(Stepper_Motors_position PRIVATE stepper.c)

target_link_libraries(Stepper_Motors_position PRIVATE pico_stdlib pico_bootsel_via_double_reset hardware_pio hardware_dma hardware_irq)

pico_add_extra_outputs(Stepper_Motors_position)
