set(PICO_BOARD pico_w CACHE STRING "Board type")

# cmake version
cmake_minimum_required(VERSION 3.13)

# include the sdk.cmake file
include(pico_sdk_import.cmake)

# give the project a name (anything you want)
project(GATT_Server C CXX ASM)

# initialize the sdk
pico_sdk_init()

add_executable( GATT_Server )

target_sources( GATT_Server
                PRIVATE
                server_demo.c
                VGA/vga16_graphics_v2.c)

# must match with pio filename and executable name from above
pico_generate_pio_header(GATT_Server ${CMAKE_CURRENT_LIST_DIR}/VGA/hsync.pio)
pico_generate_pio_header(GATT_Server ${CMAKE_CURRENT_LIST_DIR}/VGA/vsync.pio)
pico_generate_pio_header(GATT_Server ${CMAKE_CURRENT_LIST_DIR}/VGA/rgb.pio)



target_link_libraries(GATT_Server
    pico_stdlib
    pico_sync
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
    pico_bootsel_via_double_reset
    hardware_irq
    hardware_spi
    hardware_timer
    hardware_sync
    hardware_pio
    hardware_dma
    )
target_include_directories(GATT_Server PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # For btstack config
    )
pico_btstack_make_gatt_header(GATT_Server PRIVATE "${CMAKE_CURRENT_LIST_DIR}/GATT_Service/server_demo_gattfile.gatt")

pico_add_extra_outputs(GATT_Server)

